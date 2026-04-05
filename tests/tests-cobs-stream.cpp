#include "catch.hpp"
#include <Cobs.h>
#include <matrixserver.pb.h>

#include <string>
#include <vector>

// Helper: build a wire-ready buffer from a plain string (COBS-encode + append 0x00 delimiter)
static std::string makePacket(const std::string& payload) {
    std::string encoded = Cobs::encode(payload);
    encoded.push_back('\x00');
    return encoded;
}

TEST_CASE("COBS stream single complete packet", "[cobs-stream]") {
    Cobs cobs(4096);

    const std::string original = "hello world";
    const std::string packet   = makePacket(original);

    auto results = cobs.insertBytesAndReturnDecodedPackets(
        reinterpret_cast<const uint8_t*>(packet.data()), packet.size());

    REQUIRE(results.size() == 1);
    CHECK(results[0] == original);
}

TEST_CASE("COBS stream two packets in one call", "[cobs-stream]") {
    Cobs cobs(4096);

    const std::string msg1 = "first packet";
    const std::string msg2 = "second packet";
    const std::string combined = makePacket(msg1) + makePacket(msg2);

    auto results = cobs.insertBytesAndReturnDecodedPackets(
        reinterpret_cast<const uint8_t*>(combined.data()), combined.size());

    REQUIRE(results.size() == 2);
    CHECK(results[0] == msg1);
    CHECK(results[1] == msg2);
}

TEST_CASE("COBS stream split across calls", "[cobs-stream]") {
    Cobs cobs(4096);

    const std::string original = "split me across two calls";
    const std::string packet   = makePacket(original);

    // Split somewhere in the middle (not at the 0x00 delimiter)
    size_t half = packet.size() / 2;

    auto r1 = cobs.insertBytesAndReturnDecodedPackets(
        reinterpret_cast<const uint8_t*>(packet.data()), half);
    REQUIRE(r1.empty()); // no complete packet yet

    auto r2 = cobs.insertBytesAndReturnDecodedPackets(
        reinterpret_cast<const uint8_t*>(packet.data() + half), packet.size() - half);
    REQUIRE(r2.size() == 1);
    CHECK(r2[0] == original);
}

TEST_CASE("COBS stream empty input", "[cobs-stream]") {
    Cobs cobs(4096);

    auto results = cobs.insertBytesAndReturnDecodedPackets(nullptr, 0);
    CHECK(results.empty());
}

TEST_CASE("COBS stream protobuf roundtrip", "[cobs-stream]") {
    Cobs cobs(4096);

    // Build a simple MatrixServerMessage
    matrixserver::MatrixServerMessage msg;
    msg.set_messagetype(matrixserver::registerApp);
    msg.set_appid(42);

    std::string serialized;
    REQUIRE(msg.SerializeToString(&serialized));

    const std::string packet = makePacket(serialized);
    auto results = cobs.insertBytesAndReturnDecodedPackets(
        reinterpret_cast<const uint8_t*>(packet.data()), packet.size());

    REQUIRE(results.size() == 1);

    matrixserver::MatrixServerMessage decoded;
    REQUIRE(decoded.ParseFromString(results[0]));
    CHECK(decoded.messagetype() == matrixserver::registerApp);
    CHECK(decoded.appid() == 42);
}

TEST_CASE("COBS stream multiple zero delimiters", "[cobs-stream]") {
    Cobs cobs(4096);

    // Consecutive 0x00 bytes produce empty (or tiny) buffers; the decoder
    // should skip them rather than crash.
    const uint8_t zeros[] = {0x00, 0x00, 0x00};
    auto results = cobs.insertBytesAndReturnDecodedPackets(zeros, sizeof(zeros));

    // Empty packets (buffer size <= 2 bytes) must be silently skipped.
    // We only care that no valid decoded packet is returned and no crash occurs.
    for (const auto& r : results) {
        CHECK(r.empty()); // any returned entry would itself be empty
    }
}

TEST_CASE("COBS stream large packet", "[cobs-stream]") {
    Cobs cobs(65536);

    // Build ~1000 byte payload
    std::string large(1000, '\0');
    for (size_t i = 0; i < large.size(); ++i) {
        large[i] = static_cast<char>(i & 0xFF);
    }

    const std::string packet = makePacket(large);
    auto results = cobs.insertBytesAndReturnDecodedPackets(
        reinterpret_cast<const uint8_t*>(packet.data()), packet.size());

    REQUIRE(results.size() == 1);
    CHECK(results[0] == large);
}

TEST_CASE("COBS stream interleaved small packets", "[cobs-stream]") {
    Cobs cobs(4096);

    const std::vector<std::string> messages = {
        "alpha", "beta", "gamma", "delta", "epsilon"
    };

    std::vector<std::string> received;

    for (const auto& msg : messages) {
        const std::string packet = makePacket(msg);

        // Feed one byte at a time
        for (size_t i = 0; i < packet.size(); ++i) {
            const uint8_t byte = static_cast<uint8_t>(packet[i]);
            auto partial = cobs.insertBytesAndReturnDecodedPackets(&byte, 1);
            for (auto& decoded : partial) {
                received.push_back(decoded);
            }
        }
    }

    REQUIRE(received.size() == messages.size());
    for (size_t i = 0; i < messages.size(); ++i) {
        CHECK(received[i] == messages[i]);
    }
}
