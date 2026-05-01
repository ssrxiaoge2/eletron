#include "AuthService.h"

#include "../core/Config.h"
#include "../core/Database.h"
#include "../core/JwtHelper.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace Backend::Services {

AuthResult AuthService::registerUser(const QString& username, const QString& password) const
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return error(503, 2001, QStringLiteral("database_unavailable"));
    }

    QSqlQuery existsQuery(db);
    existsQuery.prepare(QStringLiteral(
        "SELECT id FROM users WHERE username = :username AND is_deleted = 0 LIMIT 1"));
    existsQuery.bindValue(QStringLiteral(":username"), username);
    if (!existsQuery.exec()) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    if (existsQuery.next()) {
        return error(409, 1004, QStringLiteral("username already exists"));
    }

    QSqlQuery insertQuery(db);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO users (username, password_hash) VALUES (:username, :password_hash)"));
    insertQuery.bindValue(QStringLiteral(":username"), username);
    insertQuery.bindValue(QStringLiteral(":password_hash"), hashPassword(password));
    if (!insertQuery.exec()) {
        const auto driverText = insertQuery.lastError().databaseText();
        if (driverText.contains(QStringLiteral("Duplicate"), Qt::CaseInsensitive)) {
            return error(409, 1004, QStringLiteral("username already exists"));
        }
        return error(503, 2002, QStringLiteral("database_error"));
    }

    AuthResult result;
    result.httpStatus = 200;
    result.data = QJsonObject {
        { QStringLiteral("userId"), insertQuery.lastInsertId().toLongLong() }
    };
    return result;
}

AuthResult AuthService::login(const QString& username, const QString& password) const
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return error(503, 2001, QStringLiteral("database_unavailable"));
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, username, password_hash FROM users "
        "WHERE username = :username AND is_deleted = 0 LIMIT 1"));
    query.bindValue(QStringLiteral(":username"), username);
    if (!query.exec()) {
        return error(503, 2002, QStringLiteral("database_error"));
    }

    if (!query.next() || query.value(QStringLiteral("password_hash")).toString() != hashPassword(password)) {
        return error(401, 1001, QStringLiteral("invalid username or password"));
    }

    const auto userId = query.value(QStringLiteral("id")).toLongLong();
    const auto foundUsername = query.value(QStringLiteral("username")).toString();
    const auto token = Core::JwtHelper::createToken(
        userId,
        foundUsername,
        Core::Config::instance().jwtSecret(),
        7);
    if (token.isEmpty()) {
        return error(503, 2003, QStringLiteral("jwt_unavailable"));
    }

    QSqlQuery sessionQuery(db);
    sessionQuery.prepare(QStringLiteral(
        "INSERT INTO sessions (user_id, token_hash, expired_at) "
        "VALUES (:user_id, :token_hash, DATE_ADD(UTC_TIMESTAMP(), INTERVAL 7 DAY))"));
    sessionQuery.bindValue(QStringLiteral(":user_id"), userId);
    sessionQuery.bindValue(QStringLiteral(":token_hash"), Core::JwtHelper::sha256Hex(token));
    if (!sessionQuery.exec()) {
        return error(503, 2002, QStringLiteral("database_error"));
    }

    AuthResult result;
    result.httpStatus = 200;
    result.data = QJsonObject {
        { QStringLiteral("token"), token },
        { QStringLiteral("userId"), userId },
        { QStringLiteral("username"), foundUsername }
    };
    return result;
}

AuthResult AuthService::error(int httpStatus, int code, const QString& message)
{
    AuthResult result;
    result.httpStatus = httpStatus;
    result.code = code;
    result.message = message;
    return result;
}

QString AuthService::hashPassword(const QString& password)
{
    return Core::JwtHelper::sha256Hex(password);
}

} // namespace Backend::Services
