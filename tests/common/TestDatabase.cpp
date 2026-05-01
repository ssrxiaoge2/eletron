#include "TestDatabase.h"

#include <QFile>
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
    return {
        QStringLiteral("SET NAMES utf8mb4"),
        QStringLiteral("SET FOREIGN_KEY_CHECKS = 0"),
        QStringLiteral("DROP TABLE IF EXISTS sessions"),
        QStringLiteral("DROP TABLE IF EXISTS messages"),
        QStringLiteral("DROP TABLE IF EXISTS friendships"),
        QStringLiteral("DROP TABLE IF EXISTS users"),
        QStringLiteral("SET FOREIGN_KEY_CHECKS = 1"),
        QStringLiteral(
            "CREATE TABLE users ("
            "id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
            "username VARCHAR(32) NOT NULL UNIQUE,"
            "password_hash VARCHAR(256) NOT NULL,"
            "nickname VARCHAR(32) DEFAULT '',"
            "status TINYINT DEFAULT 0,"
            "avatar VARCHAR(512) DEFAULT '',"
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
            "is_deleted TINYINT(1) DEFAULT 0,"
            "gender VARCHAR(16) DEFAULT '',"
            "birthday DATE NULL,"
            "signature VARCHAR(128) DEFAULT ''"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci"),
        QStringLiteral(
            "CREATE TABLE friendships ("
            "id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
            "user_id BIGINT UNSIGNED NOT NULL,"
            "friend_id BIGINT UNSIGNED NOT NULL,"
            "status TINYINT DEFAULT 0,"
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
            "is_deleted TINYINT(1) DEFAULT 0,"
            "UNIQUE KEY uk_friendships_pair (user_id, friend_id),"
            "KEY idx_friendships_user_id (user_id),"
            "KEY idx_friendships_friend_id (friend_id),"
            "CONSTRAINT fk_friendships_user_id FOREIGN KEY (user_id) REFERENCES users(id),"
            "CONSTRAINT fk_friendships_friend_id FOREIGN KEY (friend_id) REFERENCES users(id)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci"),
        QStringLiteral(
            "CREATE TABLE messages ("
            "id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
            "sender_id BIGINT UNSIGNED NOT NULL,"
            "receiver_id BIGINT UNSIGNED NOT NULL,"
            "content TEXT NOT NULL,"
            "type TINYINT DEFAULT 0,"
            "is_read TINYINT(1) DEFAULT 0,"
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
            "is_deleted TINYINT(1) DEFAULT 0,"
            "KEY idx_messages_sender_id (sender_id),"
            "KEY idx_messages_receiver_id (receiver_id),"
            "KEY idx_messages_created_at (created_at),"
            "CONSTRAINT fk_messages_sender_id FOREIGN KEY (sender_id) REFERENCES users(id),"
            "CONSTRAINT fk_messages_receiver_id FOREIGN KEY (receiver_id) REFERENCES users(id)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci"),
        QStringLiteral(
            "CREATE TABLE sessions ("
            "id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,"
            "user_id BIGINT UNSIGNED NOT NULL,"
            "token_hash VARCHAR(256) NOT NULL UNIQUE,"
            "expired_at DATETIME NOT NULL,"
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
            "is_deleted TINYINT(1) DEFAULT 0,"
            "KEY idx_sessions_user_id (user_id),"
            "KEY idx_sessions_expired_at (expired_at),"
            "CONSTRAINT fk_sessions_user_id FOREIGN KEY (user_id) REFERENCES users(id)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci")
    };
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
        QStringLiteral("DELETE FROM messages"),
        QStringLiteral("DELETE FROM friendships"),
        QStringLiteral("DELETE FROM users"),
        QStringLiteral("SET FOREIGN_KEY_CHECKS = 1")
    });
}
