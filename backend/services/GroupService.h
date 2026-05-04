#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace Backend::Services {

struct GroupResult {
    int httpStatus = 200;
    int code = 0;
    QString message = QStringLiteral("ok");
    QJsonValue data;
};

class GroupService final {
public:
    GroupResult create(const QString& bearerToken, const QJsonObject& body) const;
    GroupResult dissolve(const QString& bearerToken, qint64 groupId) const;
    GroupResult updateName(const QString& bearerToken, qint64 groupId, const QJsonObject& body) const;
    GroupResult updateAnnouncement(const QString& bearerToken, qint64 groupId, const QJsonObject& body) const;
    GroupResult info(const QString& bearerToken, qint64 groupId) const;
    GroupResult members(const QString& bearerToken, qint64 groupId) const;
    GroupResult setRole(const QString& bearerToken, qint64 groupId, qint64 userId, const QJsonObject& body) const;
    GroupResult kick(const QString& bearerToken, qint64 groupId, qint64 userId) const;
    GroupResult mute(const QString& bearerToken, qint64 groupId, qint64 userId, const QJsonObject& body) const;
    GroupResult leave(const QString& bearerToken, qint64 groupId) const;
    GroupResult sendMessage(const QString& bearerToken, qint64 groupId, const QJsonObject& body) const;
    GroupResult history(const QString& bearerToken, qint64 groupId, int page, int pageSize) const;
    GroupResult listMine(const QString& bearerToken) const;

    static bool authenticate(const QString& bearerToken, qint64* userId);
    static GroupResult error(int httpStatus, int code, const QString& message);
};

} // namespace Backend::Services
