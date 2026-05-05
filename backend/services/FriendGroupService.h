#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace Backend::Services {

struct FriendGroupResult {
    int httpStatus = 200;
    int code = 0;
    QString message = QStringLiteral("ok");
    QJsonValue data;
};

class FriendGroupService final {
public:
    FriendGroupResult list(const QString& bearerToken) const;
    FriendGroupResult create(const QString& bearerToken, const QJsonObject& body) const;
    FriendGroupResult rename(const QString& bearerToken, qint64 groupId, const QJsonObject& body) const;
    FriendGroupResult remove(const QString& bearerToken, qint64 groupId) const;
    FriendGroupResult moveFriend(const QString& bearerToken, qint64 friendUserId, const QJsonObject& body) const;

    static bool authenticate(const QString& bearerToken, qint64* userId);
    static FriendGroupResult error(int httpStatus, int code, const QString& message);
};

} // namespace Backend::Services
