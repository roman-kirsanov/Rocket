/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <mutex>
#include <ranges>
#include <thread>
#include <string>
#include <vector>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <Rocket/Fetch/Fetch.hpp>
#include <gtest/gtest.h>

using namespace Rocket;
using namespace std::chrono_literals;

namespace {

/** A small HTTP/1.1 server on 127.0.0.1 answering a fixed set of paths. */
class Server {
public:
    Server() {
        _socket = ::socket(AF_INET, SOCK_STREAM, 0);

        auto address = sockaddr_in{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;

        ::bind(_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address));
        ::listen(_socket, 16);

        auto length = socklen_t(sizeof(address));
        ::getsockname(_socket, reinterpret_cast<sockaddr*>(&address), &length);
        _port = ntohs(address.sin_port);

        _thread = std::thread([this] {
            while (true) {
                auto client = ::accept(_socket, nullptr, nullptr);

                if (client < 0) {
                    break;
                }

                auto lock = std::lock_guard(_mutex);
                _clients.emplace_back([this, client] { serve(client); });
            }
        });
    }

    ~Server() {
        ::shutdown(_socket, SHUT_RDWR);
        ::close(_socket);
        _thread.join();

        for (auto& client : _clients) {
            client.join();
        }
    }

    std::string url(std::string const& path) const {
        return "http://127.0.0.1:" + std::to_string(_port) + path;
    }
private:
    int _socket = -1;
    int _port = 0;
    std::thread _thread;
    std::mutex _mutex;
    std::vector<std::thread> _clients;

    static void serve(int client) {
        auto data = std::string();
        auto buffer = std::array<char, 4096>();

        auto receive = [&] {
            auto count = ::recv(client, buffer.data(), buffer.size(), 0);

            if (count <= 0) {
                return false;
            }

            data.append(buffer.data(), count);
            return true;
        };

        while (data.find("\r\n\r\n") == std::string::npos) {
            if (!receive()) {
                ::close(client);
                return;
            }
        }

        auto headerEnd = data.find("\r\n\r\n") + 4;
        auto head = data.substr(0, headerEnd);
        auto method = head.substr(0, head.find(' '));
        auto path = head.substr(method.size() + 1, head.find(' ', method.size() + 1) - method.size() - 1);
        auto body = std::string();
        auto rest = data.substr(headerEnd);

        if (auto at = head.find("Content-Length: "); at != std::string::npos) {
            auto length = std::stoul(head.substr(at + 16));

            while (rest.size() < length && receive()) {
                rest = data.substr(headerEnd);
            }

            body = rest.substr(0, length);
        }

        auto respond = [&](std::string const& status, std::string const& headers, std::string const& content) {
            auto response = "HTTP/1.1 " + status + "\r\nConnection: close\r\nContent-Length: " + std::to_string(content.size()) + "\r\n" + headers + "\r\n" + content;
            ::send(client, response.data(), response.size(), 0);
        };

        if (path == "/text") {
            respond("200 OK", "Content-Type: text/plain\r\nX-Test: a\r\nx-test: b\r\n", "hello");
        } else if (path == "/json") {
            respond("200 OK", "Content-Type: application/json\r\n", R"({"name":"Rocket","items":[1,2,3]})");
        } else if (path == "/echo") {
            respond("200 OK", "", method + "\n" + body);
        } else if (path == "/redirect") {
            respond("302 Found", "Location: /text\r\n", "moved");
        } else if (path == "/missing") {
            respond("404 Not Found", "", "nope");
        } else if (path == "/big") {
            respond("200 OK", "", std::string(1'000'000, 'x'));
        } else if (path == "/slow") {
            auto head = std::string("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 5\r\n\r\n");
            ::send(client, head.data(), head.size(), 0);
            std::this_thread::sleep_for(2s);
            ::send(client, "late!", 5, 0);
        } else {
            respond("500 Internal Server Error", "", "");
        }

        ::close(client);
    }
};

} // namespace

TEST(Fetch, ReadsStatusHeadersAndString) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/text") });

    EXPECT_EQ(fetch.getStatus(), 200);
    EXPECT_TRUE(fetch.isOK());
    EXPECT_EQ(fetch.getURL(), server.url("/text"));
    EXPECT_EQ(fetch.getHeader("content-type"), "text/plain");
    EXPECT_EQ(fetch.getHeader("X-TEST"), "a, b");
    EXPECT_EQ(fetch.getHeader("X-Missing"), std::nullopt);
    EXPECT_EQ(fetch.getString(), "hello");
}

TEST(Fetch, KeepsTheBodyForEveryGetter) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/text") });

    auto const& string = fetch.getString();
    auto const& bytes = fetch.getBytes();

    EXPECT_EQ(&string, &fetch.getString());
    EXPECT_EQ(bytes, std::vector<std::uint8_t>({ 'h', 'e', 'l', 'l', 'o' }));
}

TEST(Fetch, ParsesJSON) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/json") });
    auto const& json = fetch.getJSON();

    EXPECT_EQ(&json, &fetch.getJSON());
    EXPECT_EQ(std::any_cast<std::string>(std::any_cast<JSONObject const&>(json).at("name")), "Rocket");
}

TEST(Fetch, MovesOutOfATemporary) {
    auto server = Server();
    auto json = Fetch({ .url = server.url("/json") }).getJSON();
    auto string = Fetch({ .url = server.url("/text") }).getString();
    auto bytes = Fetch({ .url = server.url("/text") }).getBytes();

    EXPECT_EQ(std::any_cast<JSONArray const&>(std::any_cast<JSONObject const&>(json).at("items")).size(), 3uz);
    EXPECT_EQ(string, "hello");
    EXPECT_EQ(bytes.size(), 5uz);
}

TEST(Fetch, StreamsWithRead) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/big") });
    auto buffer = std::array<std::uint8_t, 7000>();
    auto total = 0uz;

    while (auto count = fetch.read(buffer)) {
        total += count;
    }

    EXPECT_EQ(total, 1'000'000uz);
    EXPECT_EQ(fetch.read(buffer), 0uz);
    EXPECT_THROW(fetch.getString(), FetchError);
}

TEST(Fetch, IteratesChunks) {
    static_assert(std::ranges::input_range<Fetch>);

    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/big") });
    auto total = 0uz;

    for (auto const& chunk : fetch) {
        EXPECT_FALSE(chunk.empty());
        EXPECT_EQ(chunk.front(), 'x');
        total += chunk.size();
    }

    EXPECT_EQ(total, 1'000'000uz);
}

TEST(Fetch, IteratesALoadedBodyAsOneChunk) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/text") });
    auto chunks = std::vector<std::string>();

    fetch.getString();

    for (auto const& chunk : fetch) {
        chunks.emplace_back(chunk.begin(), chunk.end());
    }

    EXPECT_EQ(chunks, std::vector<std::string>({ "hello" }));
}

TEST(Fetch, IterationContinuesAfterRead) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/text") });
    auto buffer = std::array<std::uint8_t, 2>();
    auto rest = std::string();

    EXPECT_EQ(fetch.read(buffer), 2uz);

    for (auto const& chunk : fetch) {
        rest.append(chunk.begin(), chunk.end());
    }

    EXPECT_EQ(rest, "llo");
}

TEST(Fetch, ReadsALoadedBody) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/text") });
    auto buffer = std::array<std::uint8_t, 3>();

    fetch.getString();

    EXPECT_EQ(fetch.read(buffer), 3uz);
    EXPECT_EQ(fetch.read(buffer), 2uz);
    EXPECT_EQ(fetch.read(buffer), 0uz);
}

TEST(Fetch, SendsAStringBody) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/echo"), .method = "PUT", .body = "payload" });

    EXPECT_EQ(fetch.getString(), "PUT\npayload");
}

TEST(Fetch, FollowsRedirects) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/redirect") });

    EXPECT_EQ(fetch.getStatus(), 200);
    EXPECT_EQ(fetch.getURL(), server.url("/text"));
    EXPECT_EQ(fetch.getString(), "hello");
}

TEST(Fetch, ReturnsTheRedirectWhenNotFollowing) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/redirect"), .followRedirects = false });

    EXPECT_EQ(fetch.getStatus(), 302);
    EXPECT_EQ(fetch.getHeader("Location"), "/text");
    EXPECT_EQ(fetch.getString(), "moved");
}

TEST(Fetch, ErrorStatusIsNotAnError) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/missing") });

    EXPECT_EQ(fetch.getStatus(), 404);
    EXPECT_FALSE(fetch.isOK());
    EXPECT_EQ(fetch.getString(), "nope");
}

TEST(Fetch, ThrowsWhenTheConnectionFails) {
    EXPECT_THROW(Fetch({ .url = "http://127.0.0.1:1/" }), FetchError);
}

TEST(Fetch, RejectsABodyOnGet) {
    EXPECT_THROW(Fetch({ .url = "http://127.0.0.1:1/", .body = "x" }), FetchError);
}

TEST(Fetch, Aborts) {
    auto server = Server();
    auto signal = std::make_shared<std::atomic<bool>>(false);
    auto fetch = Fetch({ .url = server.url("/slow"), .signal = signal });

    auto aborter = std::thread([&] {
        std::this_thread::sleep_for(200ms);
        signal->store(true);
    });

    auto start = std::chrono::steady_clock::now();

    EXPECT_THROW(fetch.getString(), FetchError);
    EXPECT_LT(std::chrono::steady_clock::now() - start, 1s);

    aborter.join();
}

TEST(Fetch, TimesOut) {
    auto server = Server();
    auto fetch = Fetch({ .url = server.url("/slow"), .timeout = 300ms });

    EXPECT_THROW(fetch.getString(), FetchError);
}
