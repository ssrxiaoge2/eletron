#include "UserService.h"

#include "../models/MessageModel.h"
#include "../models/UserModel.h"

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QUuid>

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

UserResult UserService::uploadAvatar(const QString& bearerToken, const QJsonObject& body) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }

    auto avatarBase64 = body.value(QStringLiteral("avatar")).toString().trimmed();
    if (avatarBase64.isEmpty()) {
        avatarBase64 = body.value(QStringLiteral("imageBase64")).toString().trimmed();
    }
    if (avatarBase64.isEmpty()) {
        return error(400, 1000, QStringLiteral("avatar is required"));
    }

    QString extension = QStringLiteral("png");
    const auto commaIndex = avatarBase64.indexOf(QLatin1Char(','));
    if (avatarBase64.startsWith(QStringLiteral("data:image/"), Qt::CaseInsensitive) && commaIndex > 0) {
        const auto meta = avatarBase64.left(commaIndex);
        if (meta.contains(QStringLiteral("jpeg"), Qt::CaseInsensitive)
            || meta.contains(QStringLiteral("jpg"), Qt::CaseInsensitive)) {
            extension = QStringLiteral("jpg");
        } else if (meta.contains(QStringLiteral("webp"), Qt::CaseInsensitive)) {
            extension = QStringLiteral("webp");
        }
        avatarBase64 = avatarBase64.mid(commaIndex + 1);
    }

    const auto bytes = QByteArray::fromBase64(avatarBase64.toUtf8());
    if (bytes.isEmpty() || bytes.size() > 2 * 1024 * 1024) {
        return error(400, 1000, QStringLiteral("invalid avatar data"));
    }

    QDir uploadDir(QDir::current().filePath(QStringLiteral("backend/uploads/avatars")));
    if (!uploadDir.exists() && !uploadDir.mkpath(QStringLiteral("."))) {
        return error(503, 2001, QStringLiteral("avatar_storage_unavailable"));
    }

    const auto filename = QStringLiteral("user_%1_%2.%3")
                              .arg(userId)
                              .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))
                              .arg(extension);
    const auto filePath = uploadDir.filePath(filename);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()) {
        return error(503, 2001, QStringLiteral("avatar_storage_unavailable"));
    }
    file.close();

    const auto avatarUrl = QStringLiteral("/uploads/avatars/%1").arg(filename);
    if (!Models::UserRepository::updateAvatar(userId, avatarUrl)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }

    UserResult result;
    result.data = QJsonObject {
        { QStringLiteral("avatar"), avatarUrl },
        { QStringLiteral("avatarUrl"), avatarUrl }
    };
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
