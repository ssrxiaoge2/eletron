#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QByteArray>
#include <QJsonArray>
#include <QObject>
#include <QTest>

static QString utf8HexMessage(const char* hex)
{
    return QString::fromUtf8(QByteArray::fromHex(hex));
}

class MessageTest : public QObject {
    Q_OBJECT
private:
    ApiClient client;
    QMap<QString, TestUser> users;
private slots:
    void init();
    void cleanup();
    void testSendTextMessage();
    void testSendMessageToStranger();
    void testGetMessages();
    void testGetMessagesPagination();
    void testGetMessagesPaginationPage2();
    void testGetConversations();
    void testConversationLastMessage();
};

void MessageTest::init()
{
    QVERIFY2(TestDatabase::initDb(), "initDb failed");
    users = createDefaultUsers(client);
    QVERIFY(users["A"].userId > 0);
    QVERIFY(users["B"].userId > 0);
    QVERIFY(makeFriends(client, users["A"], users["B"]));
}

void MessageTest::cleanup() { QVERIFY2(TestDatabase::cleanDb(), "cleanDb failed"); }

void MessageTest::testSendTextMessage()
{
    const auto response = client.post(QStringLiteral("/api/v1/messages"), {
        { QStringLiteral("receiverId"), users["B"].userId },
        { QStringLiteral("content"), utf8HexMessage("e4bda0e5a5bd") },
        { QStringLiteral("type"), 0 }
    }, users["A"].token);
    const auto data = response.data().toObject();
    QCOMPARE(response.code(), 0);
    QVERIFY(data.value(QStringLiteral("messageId")).toVariant().toLongLong() > 0);
    QVERIFY(!data.value(QStringLiteral("createdAt")).toString().isEmpty());
}

void MessageTest::testSendMessageToStranger()
{
    const auto response = client.post(QStringLiteral("/api/v1/messages"), {
        { QStringLiteral("receiverId"), users["C"].userId },
        { QStringLiteral("content"), QStringLiteral("hello stranger") },
        { QStringLiteral("type"), 0 }
    }, users["A"].token);
    QVERIFY(response.code() != 0);
}

void MessageTest::testGetMessages()
{
    QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {{QStringLiteral("receiverId"), users["B"].userId}, {QStringLiteral("content"), QStringLiteral("A to B")}, {QStringLiteral("type"), 0}}, users["A"].token).code(), 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {{QStringLiteral("receiverId"), users["A"].userId}, {QStringLiteral("content"), QStringLiteral("B to A")}, {QStringLiteral("type"), 0}}, users["B"].token).code(), 0);
    const auto response = client.get(QStringLiteral("/api/v1/messages?targetUserId=%1&page=1&pageSize=20").arg(users["B"].userId), users["A"].token);
    const auto messages = response.data().toObject().value(QStringLiteral("list")).toArray();
    QCOMPARE(response.code(), 0);
    QCOMPARE(messages.size(), 2);
    QVERIFY(messages.at(0).toObject().contains(QStringLiteral("isSelf")));
    QCOMPARE(messages.at(0).toObject().value(QStringLiteral("isSelf")).toBool(), true);
    QCOMPARE(messages.at(1).toObject().value(QStringLiteral("isSelf")).toBool(), false);
}

void MessageTest::testGetMessagesPagination()
{
    for (int i = 0; i < 25; ++i) {
        QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {{QStringLiteral("receiverId"), users["B"].userId}, {QStringLiteral("content"), QStringLiteral("message %1").arg(i)}, {QStringLiteral("type"), 0}}, users["A"].token).code(), 0);
    }
    const auto data = client.get(QStringLiteral("/api/v1/messages?targetUserId=%1&page=1&pageSize=20").arg(users["B"].userId), users["A"].token).data().toObject();
    QCOMPARE(data.value(QStringLiteral("list")).toArray().size(), 20);
    QCOMPARE(data.value(QStringLiteral("total")).toInt(), 25);
}

void MessageTest::testGetMessagesPaginationPage2()
{
    for (int i = 0; i < 25; ++i) {
        QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {{QStringLiteral("receiverId"), users["B"].userId}, {QStringLiteral("content"), QStringLiteral("message %1").arg(i)}, {QStringLiteral("type"), 0}}, users["A"].token).code(), 0);
    }
    const auto data = client.get(QStringLiteral("/api/v1/messages?targetUserId=%1&page=2&pageSize=20").arg(users["B"].userId), users["A"].token).data().toObject();
    QCOMPARE(data.value(QStringLiteral("list")).toArray().size(), 5);
}

void MessageTest::testGetConversations()
{
    QVERIFY(makeFriends(client, users["A"], users["C"]));
    QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {{QStringLiteral("receiverId"), users["B"].userId}, {QStringLiteral("content"), QStringLiteral("first chat")}, {QStringLiteral("type"), 0}}, users["A"].token).code(), 0);
    QTest::qWait(1100);
    QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {{QStringLiteral("receiverId"), users["C"].userId}, {QStringLiteral("content"), QStringLiteral("latest chat")}, {QStringLiteral("type"), 0}}, users["A"].token).code(), 0);
    const auto conversations = client.get(QStringLiteral("/api/v1/conversations"), users["A"].token).data().toArray();
    QVERIFY(conversations.size() >= 2);
    QCOMPARE(conversations.at(0).toObject().value(QStringLiteral("targetUserId")).toVariant().toLongLong(), users["C"].userId);
    QCOMPARE(conversations.at(0).toObject().value(QStringLiteral("lastMessage")).toString(), QStringLiteral("latest chat"));
}

void MessageTest::testConversationLastMessage()
{
    QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {{QStringLiteral("receiverId"), users["B"].userId}, {QStringLiteral("content"), QStringLiteral("old")}, {QStringLiteral("type"), 0}}, users["A"].token).code(), 0);
    QTest::qWait(1100);
    QCOMPARE(client.post(QStringLiteral("/api/v1/messages"), {{QStringLiteral("receiverId"), users["B"].userId}, {QStringLiteral("content"), QStringLiteral("new latest")}, {QStringLiteral("type"), 0}}, users["A"].token).code(), 0);
    const auto conversations = client.get(QStringLiteral("/api/v1/conversations"), users["A"].token).data().toArray();
    QCOMPARE(conversations.at(0).toObject().value(QStringLiteral("targetUserId")).toVariant().toLongLong(), users["B"].userId);
    QCOMPARE(conversations.at(0).toObject().value(QStringLiteral("lastMessage")).toString(), QStringLiteral("new latest"));
}

int runMessageTest(int argc, char** argv)
{
    MessageTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "MessageTest.moc"
