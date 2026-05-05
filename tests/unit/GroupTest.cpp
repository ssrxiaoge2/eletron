#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QByteArray>
#include <QJsonArray>
#include <QObject>
#include <QTest>

static QString utf8HexGroup(const char* hex)
{
    return QString::fromUtf8(QByteArray::fromHex(hex));
}

static QJsonObject findGroupById(const QJsonArray& array, qint64 groupId)
{
    for (const auto& item : array) {
        const auto object = item.toObject();
        if (object.value(QStringLiteral("groupId")).toVariant().toLongLong() == groupId) {
            return object;
        }
    }
    return {};
}

class GroupTest : public QObject {
    Q_OBJECT
private:
    ApiClient client;
    QMap<QString, TestUser> users;

    qint64 baseGroup(bool includeD = false);
    QJsonObject member(qint64 groupId, qint64 userId, const QString& token);
    QJsonArray members(qint64 groupId, const QString& token);
    void setRole(qint64 groupId, const TestUser& actor, const TestUser& target, int role);
private slots:
    void init();
    void cleanup();
    void testCreateGroupSuccess();
    void testCreateGroupWithStranger();
    void testCreateGroupTooFewMembers();
    void testGetGroupInfo();
    void testGetGroupMembers();
    void testGetMyGroups();
    void testGetClassifiedGroups();
    void testSetAdminSuccess();
    void testCancelAdmin();
    void testSetAdminByNonOwner();
    void testUpdateGroupName();
    void testUpdateGroupNameByNonOwner();
    void testPublishAnnouncement();
    void testPublishAnnouncementByNonOwner();
    void testKickMemberByOwner();
    void testKickMemberByAdmin();
    void testKickAdminByAdmin();
    void testKickOwnerByAdmin();
    void testKickMemberByNormalUser();
    void testMuteMemberByOwner();
    void testMutedUserCannotSendMessage();
    void testUnmuteMember();
    void testMuteAdminByAdmin();
    void testMuteOwnerByAdmin();
    void testMuteDurationTooShort();
    void testMuteDurationTooLong();
    void testSendGroupMessageSuccess();
    void testGetGroupMessages();
    void testGroupMessageIsSelfField();
    void testGetGroupMessagesPagination();
    void testNonMemberCannotSendMessage();
    void testLeaveGroupByMember();
    void testLeaveGroupByOwner();
    void testDissolveGroupByOwner();
    void testDissolveGroupByNonOwner();
};

void GroupTest::init()
{
    QVERIFY2(TestDatabase::initDb(), "initDb failed");
    users = createDefaultUsers(client);
    QVERIFY(users["A"].userId > 0);
    QVERIFY(users["D"].userId > 0);
}

void GroupTest::cleanup() { QVERIFY2(TestDatabase::cleanDb(), "cleanDb failed"); }

qint64 GroupTest::baseGroup(bool includeD)
{
    if (!makeFriends(client, users["A"], users["B"]) || !makeFriends(client, users["A"], users["C"])) {
        return 0;
    }
    if (includeD) {
        if (!makeFriends(client, users["A"], users["D"])) {
            return 0;
        }
        return createGroup(client, users["A"], {users["B"], users["C"], users["D"]}, utf8HexGroup("e6b58be8af95e7bea4"));
    }
    return createGroup(client, users["A"], {users["B"], users["C"]}, utf8HexGroup("e6b58be8af95e7bea4"));
}

QJsonArray GroupTest::members(qint64 groupId, const QString& token)
{
    return client.get(QStringLiteral("/api/v1/groups/%1/members").arg(groupId), token).data().toArray();
}

QJsonObject GroupTest::member(qint64 groupId, qint64 userId, const QString& token)
{
    return findObjectByUserId(members(groupId, token), userId);
}

void GroupTest::setRole(qint64 groupId, const TestUser& actor, const TestUser& target, int role)
{
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/role").arg(groupId).arg(target.userId),
                        {{QStringLiteral("role"), role}}, actor.token).code(), 0);
}

void GroupTest::testCreateGroupSuccess()
{
    QVERIFY(makeFriends(client, users["A"], users["B"]));
    QVERIFY(makeFriends(client, users["A"], users["C"]));
    const auto response = client.post(QStringLiteral("/api/v1/groups"), {
        {QStringLiteral("name"), utf8HexGroup("e6b58be8af95e7bea4")},
        {QStringLiteral("memberIds"), QJsonArray{users["B"].userId, users["C"].userId}}
    }, users["A"].token);
    QCOMPARE(response.code(), 0);
    QVERIFY(response.data().toObject().value(QStringLiteral("groupId")).toVariant().toLongLong() > 0);
    QCOMPARE(response.data().toObject().value(QStringLiteral("memberCount")).toInt(), 3);
}

void GroupTest::testCreateGroupWithStranger()
{
    QVERIFY(makeFriends(client, users["A"], users["B"]));
    const auto response = client.post(QStringLiteral("/api/v1/groups"), {
        {QStringLiteral("name"), QStringLiteral("with_stranger")},
        {QStringLiteral("memberIds"), QJsonArray{users["B"].userId, users["D"].userId}}
    }, users["A"].token);
    QCOMPARE(response.code(), 5001);
}

void GroupTest::testCreateGroupTooFewMembers()
{
    const auto response = client.post(QStringLiteral("/api/v1/groups"), {
        {QStringLiteral("name"), QStringLiteral("empty_group")},
        {QStringLiteral("memberIds"), QJsonArray{}}
    }, users["A"].token);
    QVERIFY(response.code() != 0);
}

void GroupTest::testGetGroupInfo()
{
    const qint64 groupId = baseGroup();
    const auto data = client.get(QStringLiteral("/api/v1/groups/%1").arg(groupId), users["A"].token).data().toObject();
    QVERIFY(data.contains(QStringLiteral("name")));
    QCOMPARE(data.value(QStringLiteral("ownerId")).toVariant().toLongLong(), users["A"].userId);
    QVERIFY(data.contains(QStringLiteral("announcement")));
    QCOMPARE(data.value(QStringLiteral("memberCount")).toInt(), 3);
}

void GroupTest::testGetGroupMembers()
{
    const qint64 groupId = baseGroup();
    const auto list = members(groupId, users["A"].token);
    QVERIFY(list.size() >= 3);
    QCOMPARE(list.first().toObject().value(QStringLiteral("userId")).toVariant().toLongLong(), users["A"].userId);
    QCOMPARE(member(groupId, users["A"].userId, users["A"].token).value(QStringLiteral("role")).toInt(), 2);
    QCOMPARE(member(groupId, users["B"].userId, users["A"].token).value(QStringLiteral("role")).toInt(), 0);
}

void GroupTest::testGetMyGroups()
{
    const qint64 groupId = baseGroup();
    QVERIFY(!findGroupById(client.get(QStringLiteral("/api/v1/groups"), users["A"].token).data().toArray(), groupId).isEmpty());
}

void GroupTest::testGetClassifiedGroups()
{
    const qint64 groupId = baseGroup();
    const auto owned = client.get(QStringLiteral("/api/v1/groups/classified"), users["A"].token).data().toObject().value(QStringLiteral("owned")).toArray();
    const auto joined = client.get(QStringLiteral("/api/v1/groups/classified"), users["B"].token).data().toObject().value(QStringLiteral("joined")).toArray();
    QCOMPARE(findGroupById(owned, groupId).value(QStringLiteral("myRole")).toInt(), 2);
    QCOMPARE(findGroupById(joined, groupId).value(QStringLiteral("myRole")).toInt(), 0);
}

void GroupTest::testSetAdminSuccess()
{
    const qint64 groupId = baseGroup();
    setRole(groupId, users["A"], users["B"], 1);
    QCOMPARE(member(groupId, users["B"].userId, users["A"].token).value(QStringLiteral("role")).toInt(), 1);
}

void GroupTest::testCancelAdmin()
{
    const qint64 groupId = baseGroup();
    setRole(groupId, users["A"], users["B"], 1);
    setRole(groupId, users["A"], users["B"], 0);
    QCOMPARE(member(groupId, users["B"].userId, users["A"].token).value(QStringLiteral("role")).toInt(), 0);
}

void GroupTest::testSetAdminByNonOwner()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/role").arg(groupId).arg(users["C"].userId), {{QStringLiteral("role"), 1}}, users["B"].token).code(), 5003);
}

void GroupTest::testUpdateGroupName()
{
    const qint64 groupId = baseGroup();
    const QString newName = utf8HexGroup("e696b0e7bea4e5908d");
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/name").arg(groupId), {{QStringLiteral("name"), newName}}, users["A"].token).code(), 0);
    QCOMPARE(client.get(QStringLiteral("/api/v1/groups/%1").arg(groupId), users["A"].token).data().toObject().value(QStringLiteral("name")).toString(), newName);
}

void GroupTest::testUpdateGroupNameByNonOwner()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/name").arg(groupId), {{QStringLiteral("name"), QStringLiteral("bad")}}, users["B"].token).code(), 5003);
}

void GroupTest::testPublishAnnouncement()
{
    const qint64 groupId = baseGroup();
    const QString announcement = utf8HexGroup("e4bb8ae697a5e5bc80e4bc9a");
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/announcement").arg(groupId), {{QStringLiteral("announcement"), announcement}}, users["A"].token).code(), 0);
    QCOMPARE(client.get(QStringLiteral("/api/v1/groups/%1").arg(groupId), users["A"].token).data().toObject().value(QStringLiteral("announcement")).toString(), announcement);
}

void GroupTest::testPublishAnnouncementByNonOwner()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/announcement").arg(groupId), {{QStringLiteral("announcement"), QStringLiteral("bad")}}, users["B"].token).code(), 5003);
}

void GroupTest::testKickMemberByOwner()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.deleteResource(QStringLiteral("/api/v1/groups/%1/members/%2").arg(groupId).arg(users["C"].userId), users["A"].token).code(), 0);
    QVERIFY(member(groupId, users["C"].userId, users["A"].token).isEmpty());
}

void GroupTest::testKickMemberByAdmin()
{
    const qint64 groupId = baseGroup();
    setRole(groupId, users["A"], users["B"], 1);
    QCOMPARE(client.deleteResource(QStringLiteral("/api/v1/groups/%1/members/%2").arg(groupId).arg(users["C"].userId), users["B"].token).code(), 0);
}

void GroupTest::testKickAdminByAdmin()
{
    const qint64 groupId = baseGroup(true);
    setRole(groupId, users["A"], users["B"], 1);
    setRole(groupId, users["A"], users["D"], 1);
    QCOMPARE(client.deleteResource(QStringLiteral("/api/v1/groups/%1/members/%2").arg(groupId).arg(users["D"].userId), users["B"].token).code(), 5003);
}

void GroupTest::testKickOwnerByAdmin()
{
    const qint64 groupId = baseGroup();
    setRole(groupId, users["A"], users["B"], 1);
    QCOMPARE(client.deleteResource(QStringLiteral("/api/v1/groups/%1/members/%2").arg(groupId).arg(users["A"].userId), users["B"].token).code(), 5003);
}

void GroupTest::testKickMemberByNormalUser()
{
    const qint64 groupId = baseGroup(true);
    QCOMPARE(client.deleteResource(QStringLiteral("/api/v1/groups/%1/members/%2").arg(groupId).arg(users["D"].userId), users["C"].token).code(), 5003);
}

void GroupTest::testMuteMemberByOwner()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/mute").arg(groupId).arg(users["B"].userId), {{QStringLiteral("duration"), 60}}, users["A"].token).code(), 0);
    QCOMPARE(member(groupId, users["B"].userId, users["A"].token).value(QStringLiteral("isMuted")).toBool(), true);
}

void GroupTest::testMutedUserCannotSendMessage()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/mute").arg(groupId).arg(users["B"].userId), {{QStringLiteral("duration"), 60}}, users["A"].token).code(), 0);
    const auto response = client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), QStringLiteral("hello")}, {QStringLiteral("type"), 0}}, users["B"].token);
    QCOMPARE(response.code(), 5005);
    QVERIFY(response.data().toObject().contains(QStringLiteral("muteUntil")));
}

void GroupTest::testUnmuteMember()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/mute").arg(groupId).arg(users["B"].userId), {{QStringLiteral("duration"), 60}}, users["A"].token).code(), 0);
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/mute").arg(groupId).arg(users["B"].userId), {{QStringLiteral("duration"), 0}}, users["A"].token).code(), 0);
    QCOMPARE(member(groupId, users["B"].userId, users["A"].token).value(QStringLiteral("isMuted")).toBool(), false);
    QCOMPARE(client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), QStringLiteral("hello")}, {QStringLiteral("type"), 0}}, users["B"].token).code(), 0);
}

void GroupTest::testMuteAdminByAdmin()
{
    const qint64 groupId = baseGroup(true);
    setRole(groupId, users["A"], users["B"], 1);
    setRole(groupId, users["A"], users["D"], 1);
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/mute").arg(groupId).arg(users["D"].userId), {{QStringLiteral("duration"), 60}}, users["B"].token).code(), 5003);
}

void GroupTest::testMuteOwnerByAdmin()
{
    const qint64 groupId = baseGroup();
    setRole(groupId, users["A"], users["B"], 1);
    QCOMPARE(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/mute").arg(groupId).arg(users["A"].userId), {{QStringLiteral("duration"), 60}}, users["B"].token).code(), 5003);
}

void GroupTest::testMuteDurationTooShort()
{
    const qint64 groupId = baseGroup();
    QVERIFY(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/mute").arg(groupId).arg(users["B"].userId), {{QStringLiteral("duration"), -1}}, users["A"].token).code() != 0);
}

void GroupTest::testMuteDurationTooLong()
{
    const qint64 groupId = baseGroup();
    QVERIFY(client.put(QStringLiteral("/api/v1/groups/%1/members/%2/mute").arg(groupId).arg(users["B"].userId), {{QStringLiteral("duration"), 86401}}, users["A"].token).code() != 0);
}

void GroupTest::testSendGroupMessageSuccess()
{
    const qint64 groupId = baseGroup();
    const auto response = client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), utf8HexGroup("e5a4a7e5aeb6e5a5bd")}, {QStringLiteral("type"), 0}}, users["A"].token);
    QCOMPARE(response.code(), 0);
    QVERIFY(response.data().toObject().value(QStringLiteral("messageId")).toVariant().toLongLong() > 0);
}

void GroupTest::testGetGroupMessages()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), QStringLiteral("group hello")}, {QStringLiteral("type"), 0}}, users["A"].token).code(), 0);
    const auto msg = client.get(QStringLiteral("/api/v1/groups/%1/messages?page=1&pageSize=20").arg(groupId), users["A"].token).data().toObject().value(QStringLiteral("list")).toArray().first().toObject();
    QVERIFY(msg.contains(QStringLiteral("senderNickname")));
    QVERIFY(msg.contains(QStringLiteral("senderRole")));
    QVERIFY(msg.contains(QStringLiteral("isSelf")));
}

void GroupTest::testGroupMessageIsSelfField()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), QStringLiteral("self check")}, {QStringLiteral("type"), 0}}, users["A"].token).code(), 0);
    QCOMPARE(client.get(QStringLiteral("/api/v1/groups/%1/messages?page=1&pageSize=20").arg(groupId), users["A"].token).data().toObject().value(QStringLiteral("list")).toArray().first().toObject().value(QStringLiteral("isSelf")).toBool(), true);
    QCOMPARE(client.get(QStringLiteral("/api/v1/groups/%1/messages?page=1&pageSize=20").arg(groupId), users["B"].token).data().toObject().value(QStringLiteral("list")).toArray().first().toObject().value(QStringLiteral("isSelf")).toBool(), false);
}

void GroupTest::testGetGroupMessagesPagination()
{
    const qint64 groupId = baseGroup();
    for (int i = 0; i < 25; ++i) {
        QCOMPARE(client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), QStringLiteral("group message %1").arg(i)}, {QStringLiteral("type"), 0}}, users["A"].token).code(), 0);
    }
    const auto data = client.get(QStringLiteral("/api/v1/groups/%1/messages?page=1&pageSize=20").arg(groupId), users["A"].token).data().toObject();
    QCOMPARE(data.value(QStringLiteral("list")).toArray().size(), 20);
    QCOMPARE(data.value(QStringLiteral("total")).toInt(), 25);
}

void GroupTest::testNonMemberCannotSendMessage()
{
    const qint64 groupId = baseGroup();
    QVERIFY(client.post(QStringLiteral("/api/v1/groups/%1/messages").arg(groupId), {{QStringLiteral("content"), QStringLiteral("nope")}, {QStringLiteral("type"), 0}}, users["D"].token).code() != 0);
}

void GroupTest::testLeaveGroupByMember()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.deleteResource(QStringLiteral("/api/v1/groups/%1/members/self").arg(groupId), users["C"].token).code(), 0);
    QVERIFY(member(groupId, users["C"].userId, users["A"].token).isEmpty());
}

void GroupTest::testLeaveGroupByOwner()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.deleteResource(QStringLiteral("/api/v1/groups/%1/members/self").arg(groupId), users["A"].token).code(), 5004);
}

void GroupTest::testDissolveGroupByOwner()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.deleteResource(QStringLiteral("/api/v1/groups/%1").arg(groupId), users["A"].token).code(), 0);
    QVERIFY(client.get(QStringLiteral("/api/v1/groups/%1").arg(groupId), users["A"].token).code() != 0);
}

void GroupTest::testDissolveGroupByNonOwner()
{
    const qint64 groupId = baseGroup();
    QCOMPARE(client.deleteResource(QStringLiteral("/api/v1/groups/%1").arg(groupId), users["B"].token).code(), 5003);
}

int runGroupTest(int argc, char** argv)
{
    GroupTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "GroupTest.moc"
