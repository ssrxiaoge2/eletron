#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QObject>
#include <QTest>

class AuthTest : public QObject {
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void testRegisterSuccess();
    void testRegisterDuplicateUsername();
    void testRegisterEmptyUsername();
    void testRegisterShortPassword();
    void testLoginSuccess();
    void testLoginWrongPassword();
    void testLoginNotExistUser();
    void testTokenValidAccess();
    void testTokenInvalidAccess();
    void testTokenMissingAccess();
};

void AuthTest::init() { QVERIFY2(TestDatabase::initDb(), "initDb failed"); }
void AuthTest::cleanup() { QVERIFY2(TestDatabase::cleanDb(), "cleanDb failed"); }

void AuthTest::testRegisterSuccess()
{
    ApiClient client;
    const auto response = client.post(QStringLiteral("/api/v1/auth/register"), {
        { QStringLiteral("username"), QStringLiteral("test_a") },
        { QStringLiteral("password"), QStringLiteral("Test123456") }
    });
    QCOMPARE(response.code(), 0);
    QVERIFY(response.data().toObject().value(QStringLiteral("userId")).toVariant().toLongLong() > 0);
}

void AuthTest::testRegisterDuplicateUsername()
{
    ApiClient client;
    const QJsonObject payload {
        { QStringLiteral("username"), QStringLiteral("test_a") },
        { QStringLiteral("password"), QStringLiteral("Test123456") }
    };
    QCOMPARE(client.post(QStringLiteral("/api/v1/auth/register"), payload).code(), 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/auth/register"), payload).code(), 1004);
}

void AuthTest::testRegisterEmptyUsername()
{
    ApiClient client;
    const auto response = client.post(QStringLiteral("/api/v1/auth/register"), {
        { QStringLiteral("username"), QStringLiteral("") },
        { QStringLiteral("password"), QStringLiteral("Test123456") }
    });
    QVERIFY(response.code() != 0);
}

void AuthTest::testRegisterShortPassword()
{
    ApiClient client;
    const auto response = client.post(QStringLiteral("/api/v1/auth/register"), {
        { QStringLiteral("username"), QStringLiteral("test_a") },
        { QStringLiteral("password"), QStringLiteral("12345") }
    });
    QVERIFY(response.code() != 0);
}

void AuthTest::testLoginSuccess()
{
    ApiClient client;
    QCOMPARE(client.post(QStringLiteral("/api/v1/auth/register"), {
        { QStringLiteral("username"), QStringLiteral("test_a") },
        { QStringLiteral("password"), QStringLiteral("Test123456") }
    }).code(), 0);
    const auto response = client.post(QStringLiteral("/api/v1/auth/login"), {
        { QStringLiteral("username"), QStringLiteral("test_a") },
        { QStringLiteral("password"), QStringLiteral("Test123456") }
    });
    QCOMPARE(response.code(), 0);
    QVERIFY(!response.data().toObject().value(QStringLiteral("token")).toString().isEmpty());
    QVERIFY(response.data().toObject().value(QStringLiteral("userId")).toVariant().toLongLong() > 0);
}

void AuthTest::testLoginWrongPassword()
{
    ApiClient client;
    QCOMPARE(client.post(QStringLiteral("/api/v1/auth/register"), {
        { QStringLiteral("username"), QStringLiteral("test_a") },
        { QStringLiteral("password"), QStringLiteral("Test123456") }
    }).code(), 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/auth/login"), {
        { QStringLiteral("username"), QStringLiteral("test_a") },
        { QStringLiteral("password"), QStringLiteral("Wrong123456") }
    }).code(), 1001);
}

void AuthTest::testLoginNotExistUser()
{
    ApiClient client;
    const auto response = client.post(QStringLiteral("/api/v1/auth/login"), {
        { QStringLiteral("username"), QStringLiteral("not_exist") },
        { QStringLiteral("password"), QStringLiteral("Test123456") }
    });
    QVERIFY(response.code() != 0);
}

void AuthTest::testTokenValidAccess()
{
    ApiClient client;
    const auto user = registerAndLogin(client, QStringLiteral("test_a"), QStringLiteral("Test123456"));
    QVERIFY(!user.token.isEmpty());
    QCOMPARE(client.get(QStringLiteral("/api/v1/user/profile"), user.token).code(), 0);
}

void AuthTest::testTokenInvalidAccess()
{
    ApiClient client;
    QCOMPARE(client.get(QStringLiteral("/api/v1/user/profile"), QStringLiteral("fake-token")).code(), 1002);
}

void AuthTest::testTokenMissingAccess()
{
    ApiClient client;
    const auto code = client.get(QStringLiteral("/api/v1/user/profile")).code();
    QVERIFY(code == 1002 || code == 1003);
}

int runAuthTest(int argc, char** argv)
{
    AuthTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "AuthTest.moc"
