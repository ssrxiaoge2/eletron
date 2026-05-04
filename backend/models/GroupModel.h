#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>

namespace Backend::Models {

class GroupModel final {
public:
    static bool create(qint64 ownerId,
                       const QString& name,
                       const QList<qint64>& memberIds,
                       qint64* groupId,
                       QString* createdAt);
    static bool role(qint64 groupId, qint64 userId, int* role);
    static bool isActiveMember(qint64 groupId, qint64 userId, bool* result);
    static bool dissolve(qint64 groupId);
    static bool updateName(qint64 groupId, const QString& name);
    static bool updateAnnouncement(qint64 groupId, const QString& announcement);
    static bool info(qint64 groupId, QJsonObject* info);
    static bool members(qint64 groupId, QJsonArray* members);
    static bool setRole(qint64 groupId, qint64 userId, int role);
    static bool memberRole(qint64 groupId, qint64 userId, int* role);
    static bool removeMember(qint64 groupId, qint64 userId);
    static bool muteMember(qint64 groupId, qint64 userId, int duration);
    static bool listMine(qint64 userId, QJsonArray* groups);
};

} // namespace Backend::Models
