/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <span>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <optional>
#include <stdexcept>
#include <Rocket/Base/JSON.hpp>

namespace Rocket {

/** What to request and how. */
struct FetchOptions {
    /** The http:// or https:// URL to request. */
    std::string url;

    /** The request method, sent as written (use upper case: "GET", "POST", ...). */
    std::string method = "GET";

    /** Request headers, sent in order; a name may repeat. */
    std::vector<std::pair<std::string, std::string>> headers;

    /** The request body, any bytes. GET and HEAD take no body. */
    std::optional<std::string> body;

    /** Limit for the whole request, body included; zero means no limit. */
    std::chrono::milliseconds timeout{0};

    /** Follows up to 20 redirects when true; returns the 3xx response when false. */
    bool followRedirects = true;

    /** Aborts the request when set to true from any thread; checked about every 100 ms. */
    std::shared_ptr<std::atomic<bool>> signal;
};

/**
 * A synchronous HTTP request, backed by libcurl. The constructor sends the
 * request and blocks until the response headers arrive; the body is read
 * afterwards through one of the getters. Run it inside a Task to keep the
 * main thread free.
 *
 * The body is a stream read once, so it is kept: getBytes(), getString() and
 * getJSON() load it on first call and return the stored value after that,
 * in any order. The rvalue overloads move the value out of a temporary:
 * Fetch(options).getJSON().
 *
 * To stream the body instead of loading it whole, iterate the Fetch to get
 * the chunks the network delivers, without copying; each chunk stays valid
 * until the iterator advances:
 *
 *   for (auto const& chunk : fetch) { ... }
 *
 * Or call read() to copy the next bytes into a buffer of your size. Both
 * continue from where the other stopped. Once part of the body is streamed,
 * the getters above throw. If the body was already loaded, streaming reads
 * the stored value.
 *
 * A Fetch is used from one thread at a time.
 */
class Fetch {
public:
    /** Marks the end of the body. */
    struct Sentinel {};

    /** A single-pass input iterator over the body's chunks. */
    class Iterator {
    public:
        using value_type = std::span<std::uint8_t const>;
        using difference_type = std::ptrdiff_t;

        Iterator() = default;

        /** The current chunk; never empty before the end. */
        std::span<std::uint8_t const> operator*() const;

        /** Moves to the next chunk, blocking until it arrives. @throws FetchError */
        Iterator& operator++();
        void operator++(int);

        bool operator==(Sentinel) const;
    private:
        friend class Fetch;

        Fetch* _fetch = nullptr;
        std::span<std::uint8_t const> _chunk;
    };

    ~Fetch();

    /**
     * Sends the request and waits for the response headers.
     *
     * @throws FetchError when no response arrives.
     */
    explicit Fetch(FetchOptions const& options);

    Fetch(Fetch &&) noexcept;
    Fetch(Fetch const&) = delete;
    Fetch& operator=(Fetch &&) noexcept;
    Fetch& operator=(Fetch const&) = delete;

    /** Returns true for a 2xx status. */
    bool isOK() const;

    /** The HTTP status code, e.g. 200. */
    int getStatus() const;

    /** The URL the response came from, after any redirects. */
    std::string const& getURL() const;

    /** Response headers as received; names keep the server's case. */
    std::vector<std::pair<std::string, std::string>> const& getHeaders() const;

    /**
     * Looks a header up by name, ignoring case.
     *
     * @return The value; several headers with the name are joined with ", ".
     */
    std::optional<std::string> getHeader(std::string const& name) const;

    /** The whole body as bytes. @throws FetchError */
    std::vector<std::uint8_t> const& getBytes() const&;
    std::vector<std::uint8_t> getBytes() &&;

    /** The whole body as a string. @throws FetchError */
    std::string const& getString() const&;
    std::string getString() &&;

    /** The whole body parsed as JSON. @throws FetchError, JSONError */
    JSONValue const& getJSON() const&;
    JSONValue getJSON() &&;

    /** Waits for the first chunk of the body. @throws FetchError */
    Iterator begin();
    Sentinel end() const;

    /**
     * Reads the next bytes of the body, blocking until some arrive.
     *
     * @return The number of bytes written to the span; 0 at the end of the body.
     * @throws FetchError when the transfer fails or is aborted.
     */
    std::size_t read(std::span<std::uint8_t> buffer);
private:
    struct _Fetch* _impl = nullptr;

    static std::size_t _onHeader(char* data, std::size_t size, std::size_t count, void* user);
    static std::size_t _onWrite(char* data, std::size_t size, std::size_t count, void* user);

    void _open(FetchOptions const& options);
    void _rebind();
    void _finishHeaders() const;
    bool _hasPending() const;

    template<typename Until>
    void _pump(Until const& until) const;

    template<typename Out>
    void _drain(Out& out) const;

    std::span<std::uint8_t const> _nextChunk();
    void _consume(std::size_t count);
};

/**
 * Thrown when a request cannot complete: a network or TLS failure, a timeout,
 * too many redirects, an abort, or a body read after it was already consumed.
 * HTTP error statuses (4xx, 5xx) are not errors; check Fetch::isOK().
 */
class FetchError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

} /* namespace Rocket */
