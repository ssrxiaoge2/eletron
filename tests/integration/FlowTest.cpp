#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QObject>
#include <QTest>

class FlowTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testFullFlow();
};

void FlowTest::init()
{
    QVERIFY2(TestDatabase::initDb(), "TestDatabase::initDb() failed");
}

void FlowTest::cleanup()
{
    QVERIFY2(TestDatabase::cleanDb(), "TestDatabase::cleanDb() failed");
}

void FlowTest::testFullFlow()
{
    ApiClient client;

    const auto registerA = client.post(QStringLiteral("/api/v1/auth/register"), {
        { QStringLiteral("username"), QStringLiteral("flowUserA") },
        { QStringLiteral("password"), QStringLiteral("123456") }
    });
    QVERIFY2(registerA.code() == 0, "step 1 failed: register user A");
    const auto userAId = registerA.data().toObject().value(QStringLiteral("userId")).toVariant().toLongLong();

    const auto registerB = client.post(QStringLiteral("/api/v1/auth/register"), {
        { QStringLiteral("username"), QStringLiteral("flowUserB") },
        { QStringLiteral("password"), QStringLiteral("123456") }
    });
    QVERIFY2(registerB.code() == 0, "step 1 failed: register user B");
    const auto userBId = registerB.data().toObject().value(QStringLiteral("userId")).toVariant().toLongLong();

    const auto loginA = client.post(QStringLiteral("/api/v1/auth/login"), {
        { QStringLiteral("username"), QStringLiteral("flowUserA") },
        { QStringLiteral("password"), QStringLiteral("123456") }
    });
    QVERIFY2(loginA.code() == 0, "step 2 failed: login user A");
    const auto tokenA = loginA.data().toObject().value(QStringLiteral("token")).toString();
    QVERIFY2(!tokenA.isEmpty(), "step 2 failed: token_A is empty");

    const auto loginB = client.post(QStringLiteral("/api/v1/auth/login"), {
        { QStringLiteral("username"), QStringLiteral("flowUserB") },
        { QStringLiteral("password"), QStringLiteral("123456") }
    });
    QVERIFY2(loginB.code() == 0, "step 3 failed: login user B");
    const auto tokenB = loginB.data().toObject().value(QStringLiteral("token")).toString();
    QVERIFY2(!tokenB.isEmpty(), "step 3 failed: token_B is empty");

    const auto searchB = client.get(QStringLiteral("/api/v1/users/search?keyword=flowUserB"), tokenA);
    QVERIFY2(searchB.code() == 0 && arrayContainsUserId(searchB.data().toArray(), userBId),
             "step 4 failed: A search B");
    const auto apply = client.post(QStringLiteral("/api/v1/friends/apply"), {
        { QStringLiteral("targetUserId"), userBId }
    }, tokenA);
    QVERIFY2(apply.code() == 0, "step 4 failed: A apply B");

    const auto requests = client.get(QStringLiteral("/api/v1/friends/requests"), tokenB);
    const auto requestId = firstRequestIdFrom(requests.data().toArray(), userAId);
    QVERIFY2(requests.code() == 0 && requestId > 0, "step 5 failed: B get request list");
    const auto accept = client.post(QStringLiteral("/api/v1/friends/handle"), {
        { QStringLiteral("requestId"), requestId },
        { QStringLiteral("action"), 1 }
    }, tokenB);
    QVERIFY2(accept.code() == 0, "step 5 failed: B accept request");

    const auto friendsA = client.get(QStringLiteral("/api/v1/friends"), tokenA);
    QVERIFY2(friendsA.code() == 0 && arrayContainsUserId(friendsA.data().toArray(), userBId),
             "step 6 failed: A friend list does not contain B");

    const auto sendMessage = client.post(QStringLiteral("/api/v1/messages"), {
        { QStringLiteral("receiverId"), userBId },
        { QStringLiteral("content"), QStringLiteral("你好") },
        { QStringLiteral("type"), 0 }
    }, tokenA);
    QVERIFY2(sendMessage.code() == 0, "step 7 failed: A send message to B");

    const auto history = client.get(
        QStringLiteral("/api/v1/messages?targetUserId=%1&page=1&pageSize=20").arg(userAId),
        tokenB);
    const auto messages = history.data().toObject().value(QStringLiteral("list")).toArray();
    QVERIFY2(history.code() == 0 && !messages.isEmpty(), "step 8 failed: B get message history");
    QCOMPARE(messages.last().toObject().value(QStringLiteral("content")).toString(), QStringLiteral("你好"));

    const auto conversations = client.get(QStringLiteral("/api/v1/conversations"), tokenA);
    QVERIFY2(conversations.code() == 0 && arrayContainsUserId(conversations.data().toArray(), userBId),
             "step 9 failed: A conversation list does not contain B");
}

int runFlowTest(int argc, char** argv)
{
    FlowTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "FlowTest.moc"
