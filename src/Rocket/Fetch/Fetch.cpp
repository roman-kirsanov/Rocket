/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cctype>
#include <cstring>
#include <algorithm>
#include <string_view>
#include <curl/curl.h>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Fetch/Fetch.hpp>

namespace Rocket {

struct _Fetch {
    FetchOptions options;
    ::CURL* easy = nullptr;
    ::CURLM* multi = nullptr;
    ::curl_slist* requestHeaders = nullptr;
    char errorBuffer[CURL_ERROR_SIZE] = {};

    int status = 0;
    std::string url;
    std::vector<std::pair<std::string, std::string>> headers;
    bool headersDone = false;
    bool blockHasLocation = false;

    std::string pending;
    std::size_t pendingOffset = 0;
    bool done = false;
    ::CURLcode result = CURLE_OK;

    bool streamed = false;
    std::size_t streamOffset = 0;
    std::optional<std::vector<std::uint8_t>> bytes;
    std::optional<std::string> text;
    std::optional<JSONValue> json;

    ~_Fetch() {
        if (multi != nullptr && easy != nullptr) {
            ::curl_multi_remove_handle(multi, easy);
        }

        if (easy != nullptr) {
            ::curl_easy_cleanup(easy);
        }

        if (multi != nullptr) {
            ::curl_multi_cleanup(multi);
        }

        ::curl_slist_free_all(requestHeaders);
    }
};

static bool _EqualsIgnoreCase(std::string_view a, std::string_view b) {
    return std::ranges::equal(a, b, [](unsigned char x, unsigned char y) {
        return std::tolower(x) == std::tolower(y);
    });
}

static std::string_view _Trim(std::string_view string) {
    while (string.empty() == false && (string.front() == ' ' || string.front() == '\t')) {
        string.remove_prefix(1);
    }

    while (string.empty() == false && (string.back() == ' ' || string.back() == '\t' || string.back() == '\r' || string.back() == '\n')) {
        string.remove_suffix(1);
    }

    return string;
}

std::span<std::uint8_t const> Fetch::Iterator::operator*() const {
    PROFILE

    return _chunk;
}

Fetch::Iterator& Fetch::Iterator::operator++() {
    PROFILE

    _fetch->_consume(_chunk.size());
    _chunk = _fetch->_nextChunk();

    return *this;
}

void Fetch::Iterator::operator++(int) {
    PROFILE

    ++*this;
}

bool Fetch::Iterator::operator==(Sentinel) const {
    PROFILE

    return _chunk.empty();
}

Fetch::~Fetch() {
    PROFILE

    delete _impl;
}

Fetch::Fetch(FetchOptions const& options)
    : _impl(new _Fetch())
{
    PROFILE

    try {
        _open(options);
    } catch (...) {
        delete _impl;
        throw;
    }
}

Fetch::Fetch(Fetch && other) noexcept
    : _impl(std::exchange(other._impl, nullptr))
{
    PROFILE

    _rebind();
}

Fetch& Fetch::operator=(Fetch && other) noexcept {
    PROFILE

    if (this != &other) {
        delete _impl;
        _impl = std::exchange(other._impl, nullptr);
        _rebind();
    }

    return *this;
}

bool Fetch::isOK() const {
    PROFILE

    return _impl->status >= 200 && _impl->status < 300;
}

int Fetch::getStatus() const {
    PROFILE

    return _impl->status;
}

std::string const& Fetch::getURL() const {
    PROFILE

    return _impl->url;
}

std::vector<std::pair<std::string, std::string>> const& Fetch::getHeaders() const {
    PROFILE

    return _impl->headers;
}

std::optional<std::string> Fetch::getHeader(std::string const& name) const {
    PROFILE

    auto result = std::optional<std::string>();

    for (auto const& [key, value] : _impl->headers) {
        if (_EqualsIgnoreCase(key, name)) {
            if (result.has_value()) {
                *result += ", ";
                *result += value;
            } else {
                result = value;
            }
        }
    }

    return result;
}

std::vector<std::uint8_t> const& Fetch::getBytes() const& {
    PROFILE

    if (_impl->bytes.has_value() == false) {
        if (_impl->text.has_value()) {
            _impl->bytes.emplace(_impl->text->begin(), _impl->text->end());
        } else {
            auto bytes = std::vector<std::uint8_t>();
            _drain(bytes);
            _impl->bytes = std::move(bytes);
        }
    }

    return *_impl->bytes;
}

std::vector<std::uint8_t> Fetch::getBytes() && {
    PROFILE

    static_cast<Fetch const&>(*this).getBytes();

    return std::move(*_impl->bytes);
}

std::string const& Fetch::getString() const& {
    PROFILE

    if (_impl->text.has_value() == false) {
        if (_impl->bytes.has_value()) {
            _impl->text.emplace(_impl->bytes->begin(), _impl->bytes->end());
        } else {
            auto text = std::string();
            _drain(text);
            _impl->text = std::move(text);
        }
    }

    return *_impl->text;
}

std::string Fetch::getString() && {
    PROFILE

    static_cast<Fetch const&>(*this).getString();

    return std::move(*_impl->text);
}

JSONValue const& Fetch::getJSON() const& {
    PROFILE

    if (_impl->json.has_value() == false) {
        _impl->json = ParseJSON(getString());
    }

    return *_impl->json;
}

JSONValue Fetch::getJSON() && {
    PROFILE

    static_cast<Fetch const&>(*this).getJSON();

    return std::move(*_impl->json);
}

Fetch::Iterator Fetch::begin() {
    PROFILE

    auto iterator = Iterator();
    iterator._fetch = this;
    iterator._chunk = _nextChunk();

    return iterator;
}

Fetch::Sentinel Fetch::end() const {
    PROFILE

    return {};
}

std::size_t Fetch::read(std::span<std::uint8_t> buffer) {
    PROFILE

    if (buffer.empty()) {
        return 0;
    }

    auto chunk = _nextChunk();
    auto count = std::min(buffer.size(), chunk.size());

    std::memcpy(buffer.data(), chunk.data(), count);
    _consume(count);

    return count;
}

std::size_t Fetch::_onHeader(char* data, std::size_t size, std::size_t count, void* user) {
    PROFILE

    auto& fetch = *static_cast<Fetch*>(user);
    auto length = size * count;

    if (fetch._impl->headersDone) {
        return length;
    }

    auto line = std::string_view(data, length);

    while (line.empty() == false && (line.back() == '\r' || line.back() == '\n')) {
        line.remove_suffix(1);
    }

    if (line.starts_with("HTTP/")) {
        fetch._impl->headers.clear();
        fetch._impl->blockHasLocation = false;
    } else if (line.empty()) {
        auto code = 0l;
        ::curl_easy_getinfo(fetch._impl->easy, CURLINFO_RESPONSE_CODE, &code);

        auto followed = fetch._impl->options.followRedirects && code >= 300 && code < 400 && fetch._impl->blockHasLocation;

        if (code >= 200 && followed == false) {
            fetch._finishHeaders();
        }
    } else if (line.front() == ' ' || line.front() == '\t') {
        if (fetch._impl->headers.empty() == false) {
            fetch._impl->headers.back().second += ' ';
            fetch._impl->headers.back().second += _Trim(line);
        }
    } else if (auto colon = line.find(':'); colon != std::string_view::npos) {
        auto name = line.substr(0, colon);

        if (_EqualsIgnoreCase(name, "Location")) {
            fetch._impl->blockHasLocation = true;
        }

        fetch._impl->headers.emplace_back(std::string(name), std::string(_Trim(line.substr(colon + 1))));
    }

    return length;
}

std::size_t Fetch::_onWrite(char* data, std::size_t size, std::size_t count, void* user) {
    PROFILE

    auto& fetch = *static_cast<Fetch*>(user);
    auto length = size * count;

    if (fetch._impl->headersDone == false) {
        fetch._finishHeaders();
    }

    fetch._impl->pending.append(data, length);

    return length;
}

void Fetch::_open(FetchOptions const& options) {
    PROFILE

    static auto const init = ::curl_global_init(CURL_GLOBAL_DEFAULT);

    if (init != CURLE_OK) {
        throw FetchError(::curl_easy_strerror(init));
    }

    auto hasBody = options.body.has_value();

    if (hasBody && (options.method == "GET" || options.method == "HEAD")) {
        throw FetchError("A " + options.method + " request cannot have a body");
    }

    _impl->options = options;
    _impl->easy = ::curl_easy_init();
    _impl->multi = ::curl_multi_init();

    if (_impl->easy == nullptr || _impl->multi == nullptr) {
        throw FetchError("Failed to create a libcurl handle");
    }

    ::curl_easy_setopt(_impl->easy, CURLOPT_URL, _impl->options.url.c_str());
    ::curl_easy_setopt(_impl->easy, CURLOPT_PROTOCOLS_STR, "http,https");
    ::curl_easy_setopt(_impl->easy, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
    ::curl_easy_setopt(_impl->easy, CURLOPT_FOLLOWLOCATION, options.followRedirects ? CURLFOLLOW_OBEYCODE : 0l);
    ::curl_easy_setopt(_impl->easy, CURLOPT_MAXREDIRS, 20l);
    ::curl_easy_setopt(_impl->easy, CURLOPT_ERRORBUFFER, _impl->errorBuffer);
    ::curl_easy_setopt(_impl->easy, CURLOPT_NOSIGNAL, 1l);
    ::curl_easy_setopt(_impl->easy, CURLOPT_ACCEPT_ENCODING, "");
    ::curl_easy_setopt(_impl->easy, CURLOPT_SUPPRESS_CONNECT_HEADERS, 1l);
    ::curl_easy_setopt(_impl->easy, CURLOPT_TIMEOUT_MS, static_cast<long>(options.timeout.count()));
    ::curl_easy_setopt(_impl->easy, CURLOPT_HEADERFUNCTION, _onHeader);
    ::curl_easy_setopt(_impl->easy, CURLOPT_HEADERDATA, this);
    ::curl_easy_setopt(_impl->easy, CURLOPT_WRITEFUNCTION, _onWrite);
    ::curl_easy_setopt(_impl->easy, CURLOPT_WRITEDATA, this);

    auto hasContentType = false;
    auto hasExpect = false;

    auto addHeader = [&](std::string const& line) {
        auto list = ::curl_slist_append(_impl->requestHeaders, line.c_str());

        if (list == nullptr) {
            throw FetchError("Failed to add a request header");
        }

        _impl->requestHeaders = list;
    };

    for (auto const& [name, value] : options.headers) {
        hasContentType = hasContentType || _EqualsIgnoreCase(name, "Content-Type");
        hasExpect = hasExpect || _EqualsIgnoreCase(name, "Expect");

        // curl sends "Name;" as a header with an empty value; "Name:" would remove it.
        addHeader(value.empty() ? name + ";" : name + ": " + value);
    }

    if (hasBody && hasContentType == false) {
        addHeader("Content-Type:");
    }

    if (hasBody && hasExpect == false) {
        addHeader("Expect:");
    }

    ::curl_easy_setopt(_impl->easy, CURLOPT_HTTPHEADER, _impl->requestHeaders);

    if (_impl->options.body.has_value()) {
        ::curl_easy_setopt(_impl->easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<::curl_off_t>(_impl->options.body->size()));
        ::curl_easy_setopt(_impl->easy, CURLOPT_POSTFIELDS, _impl->options.body->data());
    } else if (options.method == "POST") {
        ::curl_easy_setopt(_impl->easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<::curl_off_t>(0));
        ::curl_easy_setopt(_impl->easy, CURLOPT_POSTFIELDS, "");
    }

    if (options.method == "HEAD") {
        ::curl_easy_setopt(_impl->easy, CURLOPT_NOBODY, 1l);
    } else if (options.method != "GET" && options.method != "POST") {
        ::curl_easy_setopt(_impl->easy, CURLOPT_CUSTOMREQUEST, _impl->options.method.c_str());
    }

    ::curl_multi_add_handle(_impl->multi, _impl->easy);

    _pump([&] { return _impl->headersDone; });
}

void Fetch::_rebind() {
    PROFILE

    if (_impl != nullptr && _impl->easy != nullptr) {
        ::curl_easy_setopt(_impl->easy, CURLOPT_HEADERDATA, this);
        ::curl_easy_setopt(_impl->easy, CURLOPT_WRITEDATA, this);
    }
}

void Fetch::_finishHeaders() const {
    PROFILE

    auto code = 0l;
    char* url = nullptr;

    ::curl_easy_getinfo(_impl->easy, CURLINFO_RESPONSE_CODE, &code);
    ::curl_easy_getinfo(_impl->easy, CURLINFO_EFFECTIVE_URL, &url);

    _impl->status = static_cast<int>(code);
    _impl->url = (url != nullptr) ? url : _impl->options.url;
    _impl->headersDone = true;
}

bool Fetch::_hasPending() const {
    PROFILE

    return _impl->pending.size() > _impl->pendingOffset;
}

template<typename Until>
void Fetch::_pump(Until const& until) const {
    PROFILE

    while (until() == false && _impl->done == false) {
        if (_impl->options.signal != nullptr && _impl->options.signal->load()) {
            throw FetchError("The request was aborted");
        }

        auto running = 0;
        auto code = ::curl_multi_perform(_impl->multi, &running);

        if (code != CURLM_OK) {
            throw FetchError(::curl_multi_strerror(code));
        }

        auto queued = 0;

        while (auto message = ::curl_multi_info_read(_impl->multi, &queued)) {
            if (message->msg == CURLMSG_DONE) {
                _impl->done = true;
                _impl->result = message->data.result;
            }
        }

        if (_impl->done == false && until() == false) {
            ::curl_multi_poll(_impl->multi, nullptr, 0, 100, nullptr);
        }
    }

    if (_impl->done) {
        if (_impl->result != CURLE_OK) {
            throw FetchError((_impl->errorBuffer[0] != '\0') ? _impl->errorBuffer : ::curl_easy_strerror(_impl->result));
        }

        if (_impl->headersDone == false) {
            _finishHeaders();
        }
    }
}

template<typename Out>
void Fetch::_drain(Out& out) const {
    PROFILE

    if (_impl->streamed) {
        throw FetchError("The body was already partly streamed");
    }

    while (true) {
        _pump([&] { return _hasPending(); });

        if (_hasPending() == false) {
            break;
        }

        out.insert(out.end(), _impl->pending.begin() + _impl->pendingOffset, _impl->pending.end());
        _impl->pending.clear();
        _impl->pendingOffset = 0;
    }
}

std::span<std::uint8_t const> Fetch::_nextChunk() {
    PROFILE

    auto view = [](auto const& data, std::size_t offset) {
        return std::span(reinterpret_cast<std::uint8_t const*>(data.data()) + offset, data.size() - offset);
    };

    if (_impl->text.has_value()) {
        return view(*_impl->text, _impl->streamOffset);
    }

    if (_impl->bytes.has_value()) {
        return view(*_impl->bytes, _impl->streamOffset);
    }

    _impl->streamed = true;

    _pump([&] { return _hasPending(); });

    return view(_impl->pending, _impl->pendingOffset);
}

void Fetch::_consume(std::size_t count) {
    PROFILE


    if (_impl->text.has_value() || _impl->bytes.has_value()) {
        _impl->streamOffset += count;
        return;
    }

    _impl->pendingOffset += count;

    if (_impl->pendingOffset == _impl->pending.size()) {
        _impl->pending.clear();
        _impl->pendingOffset = 0;
    }
}

} /* namespace Rocket */
