#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QByteArray>
#include <QJsonArray>
#include <QObject>
#include <QTest>

static QString utf8HexFriend(const char* hex)
{
    return QString::fromUtf8(QByteArray::fromHex(hex));
}

class FriendTest : public QObject {
    Q_OBJECT
private:
    ApiClient client;
    QMap<QString, TestUser> users;

    qint64 createGroupForA(const QString& name);
    QJsonObject groupByNameForA(const QString& name);
private slots:
    void init();
    void cleanup();
    void testSearchByUsername();
    void testSearchByNickname();
    void testSearchNoResult();
    void testSearchExcludeSelf();
    void testApplyFriendSuccess();
    void testApplyFriendDuplicate();
    void testApplySelf();
    void testSearchFriendStatus();
    void testGetFriendRequests();
    void testAcceptFriendRequest();
    void testBidirectionalFriendship();
    void testRejectFriendRequest();
    void testGetFriendList();
    void testGetFriendGroups();
    void testCreateFriendGroup();
    void testCreateDuplicateGroupName();
    void testRenameGroup();
    void testDeleteGroup();
    void testDeleteDefaultGroup();
    void testMoveFriendToGroup();
};

void FriendTest::init()
{
    QVERIFY2(TestDatabase::initDb(), "initDb failed");
    users = createDefaultUsers(client);
    QVERIFY(users["A"].userId > 0);
    QVERIFY(users["B"].userId > 0);
    QVERIFY(users["C"].userId > 0);
}

void FriendTest::cleanup() { QVERIFY2(TestDatabase::cleanDb(), "cleanDb failed"); }

qint64 FriendTest::createGroupForA(const QString& name)
{
    const auto response = client.post(QStringLiteral("/api/v1/friend-groups"), {{QStringLiteral("name"), name}}, users["A"].token);
    return response.data().toObject().value(QStringLiteral("groupId")).toVariant().toLongLong();
}

QJsonObject FriendTest::groupByNameForA(const QString& name)
{
    return findGroupByName(client.get(QStringLiteral("/api/v1/friend-groups"), users["A"].token).data().toArray(), name);
}

void FriendTest::testSearchByUsername()
{
    const auto response = client.get(QStringLiteral("/api/v1/users/search?keyword=test_b"), users["A"].token);
    QCOMPARE(response.code(), 0);
    QVERIFY(arrayContainsUserId(response.data().toArray(), users["B"].userId));
    QVERIFY(!arrayContainsUserId(response.data().toArray(), users["A"].userId));
}

void FriendTest::testSearchByNickname()
{
    const QString nickname = utf8HexFriend("e88081e78e8b");
    QCOMPARE(client.put(QStringLiteral("/api/v1/user/profile"), {{QStringLiteral("nickname"), nickname}}, users["B"].token).code(), 0);
    const auto response = client.get(QStringLiteral("/api/v1/users/search?keyword=%1").arg(nickname), users["A"].token);
    QCOMPARE(response.code(), 0);
    QVERIFY(arrayContainsUserId(response.data().toArray(), users["B"].userId));
}

void FriendTest::testSearchNoResult()
{
    const auto response = client.get(QStringLiteral("/api/v1/users/search?keyword=xyz123_no_such_user"), users["A"].token);
    QCOMPARE(response.code(), 0);
    QVERIFY(response.data().toArray().isEmpty());
}

void FriendTest::testSearchExcludeSelf()
{
    const auto response = client.get(QStringLiteral("/api/v1/users/search?keyword=test_a"), users["A"].token);
    QCOMPARE(response.code(), 0);
    QVERIFY(!arrayContainsUserId(response.data().toArray(), users["A"].userId));
}

void FriendTest::testApplyFriendSuccess()
{
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), {{QStringLiteral("targetUserId"), users["B"].userId}}, users["A"].token).code(), 0);
}

void FriendTest::testApplyFriendDuplicate()
{
    const QJsonObject payload {{QStringLiteral("targetUserId"), users["B"].userId}};
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), payload, users["A"].token).code(), 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), payload, users["A"].token).code(), 2003);
}

void FriendTest::testApplySelf()
{
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), {{QStringLiteral("targetUserId"), users["A"].userId}}, users["A"].token).code(), 2004);
}

void FriendTest::testSearchFriendStatus()
{
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), {{QStringLiteral("targetUserId"), users["B"].userId}}, users["A"].token).code(), 0);
    const auto results = client.get(QStringLiteral("/api/v1/users/search?keyword=test_b"), users["A"].token).data().toArray();
    QCOMPARE(findObjectByUserId(results, users["B"].userId).value(QStringLiteral("friendStatus")).toInt(), 2);
}

void FriendTest::testGetFriendRequests()
{
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), {{QStringLiteral("targetUserId"), users["B"].userId}}, users["A"].token).code(), 0);
    const auto response = client.get(QStringLiteral("/api/v1/friends/requests"), users["B"].token);
    QCOMPARE(response.code(), 0);
    QVERIFY(firstRequestIdFrom(response.data().toArray(), users["A"].userId) > 0);
}

void FriendTest::testAcceptFriendRequest()
{
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), {{QStringLiteral("targetUserId"), users["B"].userId}}, users["A"].token).code(), 0);
    const qint64 requestId = firstRequestIdFrom(client.get(QStringLiteral("/api/v1/friends/requests"), users["B"].token).data().toArray(), users["A"].userId);
    QVERIFY(requestId > 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/handle"), {{QStringLiteral("requestId"), requestId}, {QStringLiteral("action"), 1}}, users["B"].token).code(), 0);
}

void FriendTest::testBidirectionalFriendship()
{
    QVERIFY(makeFriends(client, users["A"], users["B"]));
    QVERIFY(arrayContainsUserId(client.get(QStringLiteral("/api/v1/friends"), users["A"].token).data().toArray(), users["B"].userId));
    QVERIFY(arrayContainsUserId(client.get(QStringLiteral("/api/v1/friends"), users["B"].token).data().toArray(), users["A"].userId));
}

void FriendTest::testRejectFriendRequest()
{
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), {{QStringLiteral("targetUserId"), users["B"].userId}}, users["C"].token).code(), 0);
    const qint64 requestId = firstRequestIdFrom(client.get(QStringLiteral("/api/v1/friends/requests"), users["B"].token).data().toArray(), users["C"].userId);
    QVERIFY(requestId > 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/handle"), {{QStringLiteral("requestId"), requestId}, {QStringLiteral("action"), 2}}, users["B"].token).code(), 0);
    QVERIFY(!arrayContainsUserId(client.get(QStringLiteral("/api/v1/friends"), users["C"].token).data().toArray(), users["B"].userId));
}

void FriendTest::testGetFriendList()
{
    QVERIFY(makeFriends(client, users["A"], users["B"]));
    const auto response = client.get(QStringLiteral("/api/v1/friends"), users["A"].token);
    const auto friendObj = findObjectByUserId(response.data().toArray(), users["B"].userId);
    QCOMPARE(response.code(), 0);
    QVERIFY(friendObj.contains(QStringLiteral("userId")));
    QVERIFY(friendObj.contains(QStringLiteral("nickname")));
    QVERIFY(friendObj.contains(QStringLiteral("isOnline")));
}

void FriendTest::testGetFriendGroups()
{
    const QString defaultName = utf8HexFriend("e68891e79a84e5a5bde58f8b");
    const auto response = client.get(QStringLiteral("/api/v1/friend-groups"), users["A"].token);
    QCOMPARE(response.code(), 0);
    QVERIFY(!findGroupByName(response.data().toArray(), defaultName).isEmpty());
}

void FriendTest::testCreateFriendGroup()
{
    const QString groupName = utf8HexFriend("e5908ce5ada6");
    const auto response = client.post(QStringLiteral("/api/v1/friend-groups"), {{QStringLiteral("name"), groupName}}, users["A"].token);
    QCOMPARE(response.code(), 0);
    QVERIFY(response.data().toObject().value(QStringLiteral("groupId")).toVariant().toLongLong() > 0);
}

void FriendTest::testCreateDuplicateGroupName()
{
    const QString groupName = utf8HexFriend("e5908ce5ada6");
    QCOMPARE(client.post(QStringLiteral("/api/v1/friend-groups"), {{QStringLiteral("name"), groupName}}, users["A"].token).code(), 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/friend-groups"), {{QStringLiteral("name"), groupName}}, users["A"].token).code(), 6001);
}

void FriendTest::testRenameGroup()
{
    const QString oldName = utf8HexFriend("e5908ce5ada6");
    const QString newName = utf8HexFriend("e696b0e5908de5ad97");
    const qint64 groupId = createGroupForA(oldName);
    QVERIFY(groupId > 0);
    QCOMPARE(client.put(QStringLiteral("/api/v1/friend-groups/%1").arg(groupId), {{QStringLiteral("name"), newName}}, users["A"].token).code(), 0);
    QVERIFY(!groupByNameForA(newName).isEmpty());
}

void FriendTest::testDeleteGroup()
{
    QVERIFY(makeFriends(client, users["A"], users["B"]));
    const QString groupName = utf8HexFriend("e5908ce5ada6");
    const qint64 groupId = createGroupForA(groupName);
    QVERIFY(groupId > 0);
    QCOMPARE(client.put(QStringLiteral("/api/v1/friends/%1/group").arg(users["B"].userId), {{QStringLiteral("groupId"), groupId}}, users["A"].token).code(), 0);
    QCOMPARE(client.deleteResource(QStringLiteral("/api/v1/friend-groups/%1").arg(groupId), users["A"].token).code(), 0);
    QVERIFY(groupByNameForA(groupName).isEmpty());
}

void FriendTest::testDeleteDefaultGroup()
{
    const QString defaultName = utf8HexFriend("e68891e79a84e5a5bde58f8b");
    const qint64 groupId = findGroupByName(client.get(QStringLiteral("/api/v1/friend-groups"), users["A"].token).data().toArray(), defaultName)
                               .value(QStringLiteral("groupId")).toVariant().toLongLong();
    QVERIFY(groupId > 0);
    QCOMPARE(client.deleteResource(QStringLiteral("/api/v1/friend-groups/%1").arg(groupId), users["A"].token).code(), 6002);
}

void FriendTest::testMoveFriendToGroup()
{
    QVERIFY(makeFriends(client, users["A"], users["B"]));
    const QString groupName = utf8HexFriend("e5908ce5ada6");
    const qint64 groupId = createGroupForA(groupName);
    QVERIFY(groupId > 0);
    QCOMPARE(client.put(QStringLiteral("/api/v1/friends/%1/group").arg(users["B"].userId), {{QStringLiteral("groupId"), groupId}}, users["A"].token).code(), 0);
    const auto group = groupByNameForA(groupName);
    QVERIFY(arrayContainsUserId(group.value(QStringLiteral("friends")).toArray(), users["B"].userId));
}

int runFriendTest(int argc, char** argv)
{
    FriendTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "FriendTest.moc"
