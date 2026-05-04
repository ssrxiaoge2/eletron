#pragma once

#include <QJsonArray>
#include <QString>

namespace Backend::Models {

struct GroupMuteState {
    bool muted = false;
    QString muteUntil;
};

class GroupMessageModel final {
public:
    static bool muteState(qint64 groupId, qint64 userId, GroupMuteState* state);
    static bool insert(qint64 groupId,
                       qint64 senderId,
                       const QString& content,
                       int type,
                       qint64* messageId,
                       QString* createdAt);
    static bool history(qint64 groupId,
                        qint64 currentUserId,
                        int page,
                        int pageSize,
                        int* total,
                        QJsonArray* messages);
};

} // namespace Backend::Models
