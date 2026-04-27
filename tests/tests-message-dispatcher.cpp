#include "catch.hpp"
#include <MessageDispatcher.h>
#include <UniversalConnection.h>
#include <matrixserver.pb.h>

#include <atomic>
#include <memory>

// ---------------------------------------------------------------------------
// Mock UniversalConnection — implements all pure virtuals as no-ops so that
// MessageDispatcher can dispatch without needing a real transport.
// ---------------------------------------------------------------------------

class MockConnection : public UniversalConnection {
  public:
    void startReceiving() override {}
    void setReceiveCallback(std::function<void(std::shared_ptr<UniversalConnection>, std::shared_ptr<matrixserver::MatrixServerMessage>)>) override {}
    void sendMessage(std::shared_ptr<matrixserver::MatrixServerMessage>) override {}
    bool isDead() override { return false; }
    void setDead(bool) override {}
};

// ---------------------------------------------------------------------------
// 1. Register handler for one type; dispatch that type; handler fires.
// ---------------------------------------------------------------------------

TEST_CASE("MessageDispatcher dispatches registered handler for type A", "[dispatcher]") {
    MessageDispatcher dispatcher;
    std::atomic<bool> handlerCalled{false};

    dispatcher.registerHandler(
        matrixserver::registerApp,
        [&handlerCalled](std::shared_ptr<UniversalConnection>, std::shared_ptr<matrixserver::MatrixServerMessage>) { handlerCalled.store(true); });

    auto msg = std::make_shared<matrixserver::MatrixServerMessage>();
    msg->set_messagetype(matrixserver::registerApp);

    auto conn = std::make_shared<MockConnection>();
    dispatcher.dispatch(conn, msg);

    REQUIRE(handlerCalled.load() == true);
}

// ---------------------------------------------------------------------------
// 2. Two handlers for two different types — each dispatch fires the correct one.
// ---------------------------------------------------------------------------

TEST_CASE("MessageDispatcher dispatches correct handler among multiple types", "[dispatcher]") {
    MessageDispatcher dispatcher;
    std::atomic<int> typeA{0};
    std::atomic<int> typeB{0};

    dispatcher.registerHandler(matrixserver::registerApp, [&typeA](std::shared_ptr<UniversalConnection>,
                                                                   std::shared_ptr<matrixserver::MatrixServerMessage>) { typeA.fetch_add(1); });

    dispatcher.registerHandler(matrixserver::setScreenFrame, [&typeB](std::shared_ptr<UniversalConnection>,
                                                                      std::shared_ptr<matrixserver::MatrixServerMessage>) { typeB.fetch_add(1); });

    auto conn = std::make_shared<MockConnection>();

    // Dispatch type A
    auto msgA = std::make_shared<matrixserver::MatrixServerMessage>();
    msgA->set_messagetype(matrixserver::registerApp);
    dispatcher.dispatch(conn, msgA);

    // Dispatch type B
    auto msgB = std::make_shared<matrixserver::MatrixServerMessage>();
    msgB->set_messagetype(matrixserver::setScreenFrame);
    dispatcher.dispatch(conn, msgB);

    REQUIRE(typeA.load() == 1);
    REQUIRE(typeB.load() == 1);
}

// ---------------------------------------------------------------------------
// 3. Dispatch an unregistered type — no exception, no handler called.
// ---------------------------------------------------------------------------

TEST_CASE("MessageDispatcher drops unknown message type silently", "[dispatcher]") {
    MessageDispatcher dispatcher;
    std::atomic<bool> anyHandlerCalled{false};

    // Register a handler for a *different* type so the dispatcher is non-empty.
    dispatcher.registerHandler(matrixserver::registerApp,
                               [&anyHandlerCalled](std::shared_ptr<UniversalConnection>, std::shared_ptr<matrixserver::MatrixServerMessage>) {
                                   anyHandlerCalled.store(true);
                               });

    // Dispatch an unregistered type (imuData = 9).
    auto msg = std::make_shared<matrixserver::MatrixServerMessage>();
    msg->set_messagetype(matrixserver::imuData);

    auto conn = std::make_shared<MockConnection>();

    // Must not throw.
    dispatcher.dispatch(conn, msg);

    // The registered handler must NOT have been called.
    REQUIRE(anyHandlerCalled.load() == false);
}

// ---------------------------------------------------------------------------
// 4. Re-register a handler for the same type — new handler wins.
// ---------------------------------------------------------------------------

TEST_CASE("MessageDispatcher re-registering replaces old handler", "[dispatcher]") {
    MessageDispatcher dispatcher;
    std::atomic<int> oldHandler{0};
    std::atomic<int> newHandler{0};

    // First registration
    dispatcher.registerHandler(
        matrixserver::appAlive,
        [&oldHandler](std::shared_ptr<UniversalConnection>, std::shared_ptr<matrixserver::MatrixServerMessage>) { oldHandler.fetch_add(1); });

    // Re-registration — should overwrite
    dispatcher.registerHandler(
        matrixserver::appAlive,
        [&newHandler](std::shared_ptr<UniversalConnection>, std::shared_ptr<matrixserver::MatrixServerMessage>) { newHandler.fetch_add(1); });

    auto msg = std::make_shared<matrixserver::MatrixServerMessage>();
    msg->set_messagetype(matrixserver::appAlive);

    auto conn = std::make_shared<MockConnection>();
    dispatcher.dispatch(conn, msg);

    REQUIRE(oldHandler.load() == 0);
    REQUIRE(newHandler.load() == 1);
}

// ---------------------------------------------------------------------------
// 5. Handler receives the correct message and connection objects.
// ---------------------------------------------------------------------------

TEST_CASE("MessageDispatcher handler receives correct message and connection", "[dispatcher]") {
    MessageDispatcher dispatcher;
    matrixserver::MessageType receivedType;
    int32_t receivedAppId = -1;

    dispatcher.registerHandler(matrixserver::getServerInfo, [&receivedType, &receivedAppId](std::shared_ptr<UniversalConnection>,
                                                                                            std::shared_ptr<matrixserver::MatrixServerMessage> msg) {
        receivedType = msg->messagetype();
        receivedAppId = msg->appid();
    });

    auto msg = std::make_shared<matrixserver::MatrixServerMessage>();
    msg->set_messagetype(matrixserver::getServerInfo);
    msg->set_appid(42);

    auto conn = std::make_shared<MockConnection>();
    dispatcher.dispatch(conn, msg);

    REQUIRE(receivedType == matrixserver::getServerInfo);
    REQUIRE(receivedAppId == 42);
}
