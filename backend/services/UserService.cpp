#include "UserService.h"

#include "../models/MessageModel.h"
#include "../models/UserModel.h"

namespace Backend::Services {

UserResult UserService::profile(const QString& bearerToken) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }

    QJsonObject profile;
    if (!Models::UserRepository::fetchProfile(userId, &profile)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }

    UserResult result;
    result.data = profile;
    return result;
}

UserResult UserService::updateProfile(const QString& bearerToken, const QJsonObject& body) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (!validateBody(body)) {
        return error(400, 1000, QStringLiteral("invalid profile payload"));
    }

    QJsonObject updatedProfile;
    if (!Models::UserRepository::updateProfile(userId, body, &updatedProfile)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }

    UserResult result;
    result.data = updatedProfile;
    return result;
}

UserResult UserService::error(int httpStatus, int code, const QString& message)
{
    UserResult result;
    result.httpStatus = httpStatus;
    result.code = code;
    result.message = message;
    result.data = QJsonValue::Undefined;
    return result;
}

bool UserService::authenticate(const QString& bearerToken, qint64* userId)
{
    const auto trimmed = bearerToken.trimmed();
    if (!trimmed.startsWith(QStringLiteral("Bearer "), Qt::CaseInsensitive)) {
        return false;
    }

    const auto token = trimmed.mid(QStringLiteral("Bearer ").size()).trimmed();
    return Models::MessageModel::findUserIdByToken(token, userId);
}

bool UserService::validateBody(const QJsonObject& body)
{
    const auto gender = body.value(QStringLiteral("gender")).toString();
    return body.value(QStringLiteral("nickname")).toString().size() <= 32
        && body.value(QStringLiteral("signature")).toString().size() <= 128
        && body.value(QStringLiteral("avatar")).toString().size() <= 512
        && body.value(QStringLiteral("birthday")).toString().size() <= 10
        && body.value(QStringLiteral("email")).toString().size() <= 64
        && gender.size() <= 16;
}

} // namespace Backend::Services
