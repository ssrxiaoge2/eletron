#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QObject>
#include <QByteArray>
#include <QTest>

static QString utf8Hex(const char* hex)
{
    return QString::fromUtf8(QByteArray::fromHex(hex));
}

class UserTest : public QObject {
    Q_OBJECT
private:
    ApiClient client;
    QMap<QString, TestUser> users;
private slots:
    void init();
    void cleanup();
    void testGetProfileSuccess();
    void testUpdateNickname();
    void testUpdateSignature();
    void testUpdatePartialFields();
    void testUpdateEmail();
};

void UserTest::init()
{
    QVERIFY2(TestDatabase::initDb(), "initDb failed");
    users = createDefaultUsers(client);
    QVERIFY(users["A"].userId > 0);
}

void UserTest::cleanup() { QVERIFY2(TestDatabase::cleanDb(), "cleanDb failed"); }

void UserTest::testGetProfileSuccess()
{
    const auto response = client.get(QStringLiteral("/api/v1/user/profile"), users["A"].token);
    QCOMPARE(response.code(), 0);
    const auto data = response.data().toObject();
    QVERIFY(data.contains(QStringLiteral("userId")));
    QVERIFY(data.contains(QStringLiteral("username")));
    QVERIFY(data.contains(QStringLiteral("nickname")));
    QVERIFY(data.contains(QStringLiteral("signature")));
    QVERIFY(data.contains(QStringLiteral("email")));
}

void UserTest::testUpdateNickname()
{
    const QString nickname = utf8Hex("e696b0e698b5e7a7b0");
    QCOMPARE(client.put(QStringLiteral("/api/v1/user/profile"), {{QStringLiteral("nickname"), nickname}}, users["A"].token).code(), 0);
    QCOMPARE(client.get(QStringLiteral("/api/v1/user/profile"), users["A"].token).data().toObject().value(QStringLiteral("nickname")).toString(), nickname);
}

void UserTest::testUpdateSignature()
{
    const QString signature = utf8Hex("e696b0e7adbee5908d");
    QCOMPARE(client.put(QStringLiteral("/api/v1/user/profile"), {{QStringLiteral("signature"), signature}}, users["A"].token).code(), 0);
    QCOMPARE(client.get(QStringLiteral("/api/v1/user/profile"), users["A"].token).data().toObject().value(QStringLiteral("signature")).toString(), signature);
}

void UserTest::testUpdatePartialFields()
{
    const QString originalSignature = utf8Hex("e58e9fe7adbee5908d");
    const QString nickname = utf8Hex("e58faae694b9e698b5e7a7b0");
    QCOMPARE(client.put(QStringLiteral("/api/v1/user/profile"), {{QStringLiteral("signature"), originalSignature}}, users["A"].token).code(), 0);
    QCOMPARE(client.put(QStringLiteral("/api/v1/user/profile"), {{QStringLiteral("nickname"), nickname}}, users["A"].token).code(), 0);
    const auto profile = client.get(QStringLiteral("/api/v1/user/profile"), users["A"].token).data().toObject();
    QCOMPARE(profile.value(QStringLiteral("nickname")).toString(), nickname);
    QCOMPARE(profile.value(QStringLiteral("signature")).toString(), originalSignature);
}

void UserTest::testUpdateEmail()
{
    const auto response = client.put(QStringLiteral("/api/v1/user/profile"), {{QStringLiteral("email"), QStringLiteral("test@test.com")}}, users["A"].token);
    QCOMPARE(response.code(), 0);
    QCOMPARE(response.data().toObject().value(QStringLiteral("email")).toString(), QStringLiteral("test@test.com"));
}

int runUserTest(int argc, char** argv)
{
    UserTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "UserTest.moc"
