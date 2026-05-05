#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace Backend::Models {

class FriendGroupModel final {
public:
    static bool ensureDefaultGroup(qint64 userId, qint64* groupId = nullptr);
    static bool listWithFriends(qint64 userId, QJsonArray* groups);
    static bool create(qint64 userId, const QString& name, QJsonObject* group, int* businessCode);
    static bool rename(qint64 userId, qint64 groupId, const QString& name, int* businessCode);
    static bool remove(qint64 userId, qint64 groupId, int* businessCode);
    static bool moveFriend(qint64 userId, qint64 friendUserId, qint64 groupId, int* businessCode);
};

} // namespace Backend::Models
