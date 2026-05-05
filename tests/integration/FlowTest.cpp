#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QObject>
#include <QTest>

static QString utf8HexFlow(const char* hex)
{
    return QString::fromUtf8(QByteArray::fromHex(hex));
}

static QByteArray flowPngBytes()
{
    return QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==");
}

class FlowTest : public QObject {
    Q_OBJECT
private:
    ApiClient client;
private slots:
    void init();
    void cleanup();
    void testPrivateChatFullFlow();
    void testGroupChatFullFlow();
    void testFileTransferFullFlow();
    void testFriendGroupFullFlow();
};

void FlowTest::init() { QVERIFY2(TestDatabase::initDb(), "initDb failed"); }
void FlowTest::cleanup() { QVERIFY2(TestDatabase::cleanDb(), "cleanDb failed"); }

void FlowTest::testPrivateChatFullFlow()
{
    const auto userA = registerAndLogin(client, QStringLiteral("test_a"), QStringLiteral("Test123456"));
    const auto userB = registerAndLogin(client, QStringLiteral("test_b"), QStringLiteral("Test123456"));
    QVERIFY2(userA.userId > 0 && userB.userId > 0, "step 1-2 failed: register/login users");
    const auto search = client.get(QStringLiteral("/api/v1/users/search?keyword=test_b"), userA.token);
    QVERIFY2(search.code() == 0 && findObjectByUserId(search.data().toArray(), userB.userId).value(QStringLiteral("friendStatus")).toInt() == 0,
             "step 3 failed: search friendStatus");
    QVERIFY2(client.post(QStringLiteral("/api/v1/friends/apply"), {{QStringLiteral("targetUserId"), userB.userId}}, userA.token).code() == 0,
             "step 4 failed: apply friend");
    const qint64 requestId = firstRequestIdFrom(client.get(QStringLiteral("/api/v1/friends/requests"), userB.token).data().toArray(), userA.userId);
    QVERIFY2(requestId > 0, "step 5 failed: request list");
    QVERIFY2(client.post(QStringLiteral("/api/v1/friends/handle"), {{QStringLiteral("requestId"), requestId}, {QStringLiteral("action"), 1}}, userB.token).code() == 0,
             "step 6 failed: accept request");
    QVERIFY2(arrayContainsUserId(client.get(QStringLiteral("/api/v1/friends"), userA.token).data().toArray(), userB.userId),
             "step 7 failed: friend list");
    const QString hello = utf8HexFlow("e4bda0e5a5bd");
    QVERIFY2(client.post(QStringLiteral("/api/v1/messages"), {{QStringLiteral("receiverId"), userB.userId}, {QStringLiteral("content"), hello}, {QStringLiteral("type"), 0}}, userA.token).code() == 0,
             "step 8 failed: send message");
    const auto messages = client.get(QStringLiteral("/api/v1/messages?targetUserId=%1&page=1&pageSize=20").arg(userA.userId), userB.token).data().toObject().value(QStringLiteral("list")).toArray();
    QVERIFY2(!messages.isEmpty() && messages.last().toObject().value(QStringLiteral("content")).toString() == hello,
             "step 9 failed: message history");
    QVERIFY2(arrayContainsUserId(client.get(QStringLiteral("/api/v1/conversations"), userA.token).data().toArray(), userB.userId),
             "step 10 failed: conversation list");
}

void FlowTest::testGroupChatFullFlow()
{
    auto users = createDefaultUsers(client);
    QVERIFY2(makeFriends(client, users["A"], users["B"]) && makeFriends(client, users["A"], users["C"]) && makeFriends(client, users["B"], users["C"]),
             "step 1 failed: friendship setup");
    const qint64 groupId = createGroup(client, users["A"], {users["B"], users["C"]}, utf8HexFlow("e6b58be8af95e7bea4"));
    QVERIFY2(groupId > 0, "step 2 failed: create group");
    auto members = client.get(QStringLiteral("/api/v1/groups/%1/members").arg(groupId), users["A"].token).data().toArray();
    QVERIFY2(arrayContainsUserId(members, users["A"].userId) && arrayContainsUserId(members, users["B"].userId) && arrayContainsUserId(members, users["C"].userId),
             "step 3 failed: group members");
    QVERIFY2(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/role").arg(groupId).arg(users["B"].userId), {{QStringLiteral("role"), 1}}, users["A"].token).code() == 0,
             "step 4 failed: set admin");
    QVERIFY2(client.put(QStringLiteral("/api/v1/groups/%1/announcement").arg(groupId), {{QStringLiteral("announcement"), utf8HexFlow("e6b58be8af95e585ace5918a")}}, users["A"].token).code() == 0,
             "step 5 failed: publish announcement");
    QVERIFY2(client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), QStringLiteral("I am admin")}, {QStringLiteral("type"), 0}}, users["B"].token).code() == 0,
             "step 6 failed: admin message");
    QVERIFY2(client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), QStringLiteral("I am member")}, {QStringLiteral("type"), 0}}, users["C"].token).code() == 0,
             "step 7 failed: member message");
    QVERIFY2(client.get(QStringLiteral("/api/v1/groups/%1/messages?page=1&pageSize=20").arg(groupId), users["A"].token).data().toObject().value(QStringLiteral("list")).toArray().size() >= 2,
             "step 8 failed: group history");
    QVERIFY2(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/mute").arg(groupId).arg(users["C"].userId), {{QStringLiteral("duration"), 60}}, users["A"].token).code() == 0,
             "step 9 failed: mute userC");
    QVERIFY2(client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), QStringLiteral("muted")}, {QStringLiteral("type"), 0}}, users["C"].token).code() == 5005,
             "step 10 failed: muted user should be blocked");
    QVERIFY2(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/mute").arg(groupId).arg(users["C"].userId), {{QStringLiteral("duration"), 0}}, users["A"].token).code() == 0,
             "step 11 failed: unmute userC");
    QVERIFY2(client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), QStringLiteral("after unmute")}, {QStringLiteral("type"), 0}}, users["C"].token).code() == 0,
             "step 12 failed: message after unmute");
    QVERIFY2(client.deleteResource(QStringLiteral("/api/v1/groups/%1/members/%2").arg(groupId).arg(users["C"].userId), users["B"].token).code() == 0,
             "step 13 failed: admin kick userC");
    members = client.get(QStringLiteral("/api/v1/groups/%1/members").arg(groupId), users["A"].token).data().toArray();
    QVERIFY2(!arrayContainsUserId(members, users["C"].userId), "step 14 failed: userC still member");
    QVERIFY2(client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), QStringLiteral("after kick")}, {QStringLiteral("type"), 0}}, users["C"].token).code() != 0,
             "step 15 failed: kicked user can still send");
    QVERIFY2(client.deleteResource(QStringLiteral("/api/v1/groups/%1").arg(groupId), users["A"].token).code() == 0,
             "step 16 failed: dissolve group");
}

void FlowTest::testFileTransferFullFlow()
{
    auto users = createDefaultUsers(client);
    QVERIFY2(makeFriends(client, users["A"], users["B"]), "step 1 failed: make friends");
    auto image = client.postMultipart(QStringLiteral("/api/v1/files/upload"), users["A"].token, QStringLiteral("file"), QStringLiteral("tiny.png"),
                                      flowPngBytes(), QStringLiteral("image/png"), {{QStringLiteral("type"), 1}, {QStringLiteral("receiverId"), users["B"].userId}});
    QVERIFY2(image.code() == 0 && !image.data().toObject().value(QStringLiteral("thumbnailUrl")).toString().isEmpty(), "step 2-3 failed: upload image");
    auto history = client.get(QStringLiteral("/api/v1/messages?targetUserId=%1&page=1&pageSize=20").arg(users["B"].userId), users["A"].token).data().toObject().value(QStringLiteral("list")).toArray();
    QVERIFY2(!history.isEmpty() && history.last().toObject().value(QStringLiteral("type")).toInt() == 1, "step 4 failed: image history");
    QVERIFY2(client.get(QStringLiteral("/api/v1/files/download/%1").arg(image.data().toObject().value(QStringLiteral("fileId")).toVariant().toLongLong()), users["B"].token).httpStatus == 200,
             "step 5 failed: download image");
    const QByteArray fileBytes("flow text");
    auto file = client.postMultipart(QStringLiteral("/api/v1/files/upload"), users["A"].token, QStringLiteral("file"), QStringLiteral("flow.txt"),
                                     fileBytes, QStringLiteral("text/plain"), {{QStringLiteral("type"), 2}, {QStringLiteral("receiverId"), users["B"].userId}});
    QVERIFY2(file.code() == 0, "step 6 failed: upload file");
    history = client.get(QStringLiteral("/api/v1/messages?targetUserId=%1&page=1&pageSize=20").arg(users["B"].userId), users["A"].token).data().toObject().value(QStringLiteral("list")).toArray();
    QVERIFY2(!history.isEmpty() && history.last().toObject().value(QStringLiteral("type")).toInt() == 2, "step 7 failed: file history");
    QVERIFY2(client.get(QStringLiteral("/api/v1/files/download/%1").arg(file.data().toObject().value(QStringLiteral("fileId")).toVariant().toLongLong()), users["B"].token).rawBody.size() == fileBytes.size(),
             "step 8 failed: download file size");
}

void FlowTest::testFriendGroupFullFlow()
{
    auto users = createDefaultUsers(client);
    QVERIFY2(makeFriends(client, users["A"], users["B"]), "step 1 failed: make friends");
    const QString classmates = utf8HexFlow("e5908ce5ada6");
    auto create = client.post(QStringLiteral("/api/v1/friend-groups"), {{QStringLiteral("name"), classmates}}, users["A"].token);
    const qint64 groupId = create.data().toObject().value(QStringLiteral("groupId")).toVariant().toLongLong();
    QVERIFY2(create.code() == 0 && groupId > 0, "step 2 failed: create group");
    QVERIFY2(client.put(QStringLiteral("/api/v1/friends/%1/group").arg(users["B"].userId), {{QStringLiteral("groupId"), groupId}}, users["A"].token).code() == 0,
             "step 3 failed: move friend");
    QVERIFY2(arrayContainsUserId(findGroupByName(client.get(QStringLiteral("/api/v1/friend-groups"), users["A"].token).data().toArray(), classmates).value(QStringLiteral("friends")).toArray(), users["B"].userId),
             "step 4 failed: friend not in group");
    const QString newName = utf8HexFlow("e5a4a7e5ada6e5908ce5ada6");
    QVERIFY2(client.put(QStringLiteral("/api/v1/friend-groups/%1").arg(groupId), {{QStringLiteral("name"), newName}}, users["A"].token).code() == 0,
             "step 5 failed: rename group");
    QVERIFY2(!findGroupByName(client.get(QStringLiteral("/api/v1/friend-groups"), users["A"].token).data().toArray(), newName).isEmpty(),
             "step 6 failed: renamed group not found");
    QVERIFY2(client.deleteResource(QStringLiteral("/api/v1/friend-groups/%1").arg(groupId), users["A"].token).code() == 0,
             "step 7 failed: delete group");
    const QString defaultName = utf8HexFlow("e68891e79a84e5a5bde58f8b");
    QVERIFY2(arrayContainsUserId(findGroupByName(client.get(QStringLiteral("/api/v1/friend-groups"), users["A"].token).data().toArray(), defaultName).value(QStringLiteral("friends")).toArray(), users["B"].userId),
             "step 8 failed: friend not moved to default group");
}

int runFlowTest(int argc, char** argv)
{
    FlowTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "FlowTest.moc"
