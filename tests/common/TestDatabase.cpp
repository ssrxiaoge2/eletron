#include "TestDatabase.h"

#include <QFile>
#include <QRegularExpression>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

namespace {

struct DbConfig {
    QString host;
    int port = 3306;
    QString database;
    QString username;
    QString password;
};

DbConfig loadConfig()
{
    QSettings settings(QStringLiteral(TEST_DB_CONFIG_PATH), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("mysql"));
    DbConfig config;
    config.host = settings.value(QStringLiteral("host"), QStringLiteral("127.0.0.1")).toString();
    config.port = settings.value(QStringLiteral("port"), 3306).toInt();
    config.database = settings.value(QStringLiteral("database"), QStringLiteral("im_app_test")).toString();
    config.username = settings.value(QStringLiteral("username"), QStringLiteral("root")).toString();
    config.password = settings.value(QStringLiteral("password")).toString();
    settings.endGroup();
    return config;
}

QSqlDatabase openConnection(const QString& name, const DbConfig& config, const QString& database)
{
    if (QSqlDatabase::contains(name)) {
        QSqlDatabase::removeDatabase(name);
    }

    auto db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), name);
    db.setHostName(config.host);
    db.setPort(config.port);
    db.setDatabaseName(database);
    db.setUserName(config.username);
    db.setPassword(config.password);
    db.setConnectOptions(QStringLiteral("MYSQL_OPT_RECONNECT=1"));
    db.open();
    return db;
}

bool execSql(QSqlDatabase& db, const QString& sql)
{
    QSqlQuery query(db);
    return query.exec(sql);
}

bool execAll(QSqlDatabase& db, const QStringList& statements)
{
    for (const auto& sql : statements) {
        if (!execSql(db, sql)) {
            return false;
        }
    }
    return true;
}

QStringList schemaStatements()
{
    QFile file(QStringLiteral(TEST_SCHEMA_SQL_PATH));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    auto sql = QString::fromUtf8(file.readAll());
    sql.replace(QRegularExpression(QStringLiteral("\\bim_app\\b")), QStringLiteral("im_app_test"));

    QStringList statements;
    for (const auto& part : sql.split(QLatin1Char(';'))) {
        const auto statement = part.trimmed();
        if (!statement.isEmpty()) {
            statements << statement;
        }
    }
    return statements;
}

} // namespace

bool TestDatabase::initDb()
{
    const auto config = loadConfig();
    auto adminDb = openConnection(QStringLiteral("test_admin"), config, QStringLiteral("mysql"));
    if (!adminDb.isOpen()) {
        return false;
    }

    if (!execSql(adminDb,
                 QStringLiteral("CREATE DATABASE IF NOT EXISTS %1 DEFAULT CHARACTER SET utf8mb4 DEFAULT COLLATE utf8mb4_unicode_ci")
                     .arg(config.database))) {
        return false;
    }
    adminDb.close();

    auto db = openConnection(QStringLiteral("test_case"), config, config.database);
    if (!db.isOpen()) {
        return false;
    }
    return execAll(db, schemaStatements());
}

bool TestDatabase::cleanDb()
{
    const auto config = loadConfig();
    auto db = openConnection(QStringLiteral("test_cleanup"), config, config.database);
    if (!db.isOpen()) {
        return false;
    }

    return execAll(db, {
        QStringLiteral("SET FOREIGN_KEY_CHECKS = 0"),
        QStringLiteral("DELETE FROM sessions"),
        QStringLiteral("DELETE FROM group_messages"),
        QStringLiteral("DELETE FROM group_members"),
        QStringLiteral("DELETE FROM `groups`"),
        QStringLiteral("DELETE FROM files"),
        QStringLiteral("DELETE FROM messages"),
        QStringLiteral("DELETE FROM conversations"),
        QStringLiteral("DELETE FROM friendships"),
        QStringLiteral("DELETE FROM friend_groups"),
        QStringLiteral("DELETE FROM users"),
        QStringLiteral("SET FOREIGN_KEY_CHECKS = 1")
    });
}
