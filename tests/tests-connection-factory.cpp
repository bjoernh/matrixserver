#include "catch.hpp"
#include <ConnectionFactory.h>
#include <boost/asio.hpp>

// ---------------------------------------------------------------------------
// 1. Empty URI — no "://" present — must return nullptr.
// ---------------------------------------------------------------------------

TEST_CASE("ConnectionFactory empty URI returns nullptr", "[connection-factory]") {
    boost::asio::io_context ctx;
    auto result = ConnectionFactory::connectFromUri(ctx, "");
    REQUIRE(!result);
}

// ---------------------------------------------------------------------------
// 2. URI without "://" — must return nullptr.
// ---------------------------------------------------------------------------

TEST_CASE("ConnectionFactory URI without scheme separator returns nullptr", "[connection-factory]") {
    boost::asio::io_context ctx;
    auto result = ConnectionFactory::connectFromUri(ctx, "localhost:2017");
    REQUIRE(!result);
}

// ---------------------------------------------------------------------------
// 3. Unknown scheme — must return nullptr.
// ---------------------------------------------------------------------------

TEST_CASE("ConnectionFactory unknown scheme returns nullptr", "[connection-factory]") {
    boost::asio::io_context ctx;
    auto result = ConnectionFactory::connectFromUri(ctx, "garbage://x");
    REQUIRE(!result);
}

// ---------------------------------------------------------------------------
// 4. TCP to a port with no listener — must return a connection that isDead().
// ---------------------------------------------------------------------------

TEST_CASE("ConnectionFactory TCP to unreachable host returns dead connection", "[connection-factory]") {
    boost::asio::io_context ctx;
    // Port 65535 on localhost is very unlikely to be in use.
    auto result = ConnectionFactory::connectFromUri(ctx, "tcp://127.0.0.1:65535");
    REQUIRE(result);           // factory returns the SocketConnection object
    REQUIRE(result->isDead()); // but the connection failed
}

// ---------------------------------------------------------------------------
// 5. TCP URI missing port — must return nullptr.
// ---------------------------------------------------------------------------

TEST_CASE("ConnectionFactory TCP URI without port returns nullptr", "[connection-factory]") {
    boost::asio::io_context ctx;
    auto result = ConnectionFactory::connectFromUri(ctx, "tcp://localhost");
    REQUIRE(!result);
}

// ---------------------------------------------------------------------------
// 6. IPC to a nonexistent queue — connection should be dead.
// ---------------------------------------------------------------------------

TEST_CASE("ConnectionFactory IPC to nonexistent queue returns dead connection", "[connection-factory]") {
    boost::asio::io_context ctx;
    auto result = ConnectionFactory::connectFromUri(ctx, "ipc://nonexistent-queue-test-12345");
    REQUIRE(result);           // factory always returns the IpcConnection object
    REQUIRE(result->isDead()); // but connectToServer failed internally
}
