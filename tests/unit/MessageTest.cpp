#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QJsonArray>
#include <QObject>
#include <QTest>

class MessageTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testSendMessage();
    void testSendMessageToStranger();
    void testGetMessages();
    void testGetMessagesPagination();
    void testGetConversations();
};

void MessageTest::init()
{
    QVERIFY2(TestDatabase::initDb(), "TestDatabase::initDb() failed");
}

void MessageTest::cleanup()
{
    QVERIFY2(TestDatabase::cleanDb(), "TestDatabase::cleanDb() failed");
}

void MessageTest::testSendMessage()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);
    QVERIFY(makeFriends(client, userA, userB));

    const auto response = client.post(QStringLiteral("/api/v1/messages"), {
        { QStringLiteral("receiverId"), userB.userId },
        { QStringLiteral("content"), QStringLiteral("你好") },
        { QStringLiteral("type"), 0 }
    }, userA.token);
    const auto data = response.data().toObject();

    QCOMPARE(response.code(), 0);
    QVERIFY(data.value(QStringLiteral("messageId")).toVariant().toLongLong() > 0);
    QVERIFY(!data.value(QStringLiteral("createdAt")).toString().isEmpty());
}

void MessageTest::testSendMessageToStranger()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);

    const auto response = client.post(QStringLiteral("/api/v1/messages"), {
        { QStringLiteral("receiverId"), userB.userId },
        { QStringLiteral("content"), QStringLiteral("你好") },
        { QStringLiteral("type"), 0 }
    }, userA.token);

    QVERIFY(response.code() != 0);
}

void MessageTest::testGetMessages()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);
    QVERIFY(makeFriends(client, userA, userB));

    QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {
        { QStringLiteral("receiverId"), userB.userId },
        { QStringLiteral("content"), QStringLiteral("A to B") },
        { QStringLiteral("type"), 0 }
    }, userA.token).code(), 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {
        { QStringLiteral("receiverId"), userA.userId },
        { QStringLiteral("content"), QStringLiteral("B to A") },
        { QStringLiteral("type"), 0 }
    }, userB.token).code(), 0);

    const auto response = client.get(
        QStringLiteral("/api/v1/messages?targetUserId=%1&page=1&pageSize=20").arg(userA.userId),
        userB.token);
    const auto data = response.data().toObject();
    const auto messages = data.value(QStringLiteral("list")).toArray();

    QCOMPARE(response.code(), 0);
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages.at(0).toObject().value(QStringLiteral("content")).toString(), QStringLiteral("A to B"));
    QCOMPARE(messages.at(0).toObject().value(QStringLiteral("isSelf")).toBool(), false);
    QCOMPARE(messages.at(1).toObject().value(QStringLiteral("content")).toString(), QStringLiteral("B to A"));
    QCOMPARE(messages.at(1).toObject().value(QStringLiteral("isSelf")).toBool(), true);
}

void MessageTest::testGetMessagesPagination()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);
    QVERIFY(makeFriends(client, userA, userB));

    for (int i = 0; i < 25; ++i) {
        QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {
            { QStringLiteral("receiverId"), userB.userId },
            { QStringLiteral("content"), QStringLiteral("message %1").arg(i) },
            { QStringLiteral("type"), 0 }
        }, userA.token).code(), 0);
    }

    const auto response = client.get(
        QStringLiteral("/api/v1/messages?targetUserId=%1&page=1&pageSize=20").arg(userB.userId),
        userA.token);
    const auto data = response.data().toObject();

    QCOMPARE(response.code(), 0);
    QCOMPARE(data.value(QStringLiteral("list")).toArray().size(), 20);
    QCOMPARE(data.value(QStringLiteral("total")).toInt(), 25);
}

void MessageTest::testGetConversations()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    const auto userC = registerAndLogin(client, QStringLiteral("testuserC"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);
    QVERIFY(userC.userId > 0);
    QVERIFY(makeFriends(client, userA, userB));
    QVERIFY(makeFriends(client, userA, userC));

    QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {
        { QStringLiteral("receiverId"), userB.userId },
        { QStringLiteral("content"), QStringLiteral("first chat") },
        { QStringLiteral("type"), 0 }
    }, userA.token).code(), 0);
    QTest::qWait(1100);
    QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {
        { QStringLiteral("receiverId"), userC.userId },
        { QStringLiteral("content"), QStringLiteral("latest chat") },
        { QStringLiteral("type"), 0 }
    }, userA.token).code(), 0);

    const auto response = client.get(QStringLiteral("/api/v1/conversations"), userA.token);
    const auto conversations = response.data().toArray();

    QCOMPARE(response.code(), 0);
    QVERIFY(conversations.size() >= 2);
    QCOMPARE(conversations.at(0).toObject().value(QStringLiteral("targetUserId")).toVariant().toLongLong(), userC.userId);
    QCOMPARE(conversations.at(0).toObject().value(QStringLiteral("lastMessage")).toString(), QStringLiteral("latest chat"));
    QCOMPARE(conversations.at(1).toObject().value(QStringLiteral("targetUserId")).toVariant().toLongLong(), userB.userId);
    QCOMPARE(conversations.at(1).toObject().value(QStringLiteral("lastMessage")).toString(), QStringLiteral("first chat"));
}

int runMessageTest(int argc, char** argv)
{
    MessageTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "MessageTest.moc"
