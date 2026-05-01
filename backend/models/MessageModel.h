#pragma once

#include <QJsonArray>
#include <QJsonObject>

namespace Backend::Models {

class MessageModel final {
public:
    static bool findUserIdByToken(const QString& token, qint64* userId);
    static bool areFriends(qint64 userId, qint64 targetUserId, bool* result);
    static bool fetchConversations(qint64 userId, QJsonArray* conversations);
    static bool fetchHistory(qint64 userId,
                             qint64 targetUserId,
                             int page,
                             int pageSize,
                             int* total,
                             QJsonArray* messages);
    static bool insertMessage(qint64 senderId,
                              qint64 receiverId,
                              const QString& content,
                              int type,
                              qint64* messageId,
                              QString* createdAt);
};

} // namespace Backend::Models
