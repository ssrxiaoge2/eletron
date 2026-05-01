#include "MessageService.h"

#include "../models/MessageModel.h"

#include <QJsonArray>

namespace Backend::Services {

MessageResult MessageService::conversations(const QString& bearerToken) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }

    QJsonArray conversations;
    if (!Models::MessageModel::fetchConversations(userId, &conversations)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }

    MessageResult result;
    result.data = conversations;
    return result;
}

MessageResult MessageService::history(const QString& bearerToken,
                                      qint64 targetUserId,
                                      int page,
                                      int pageSize) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (targetUserId <= 0 || page <= 0 || pageSize <= 0 || pageSize > 100) {
        return error(400, 1000, QStringLiteral("invalid query parameters"));
    }

    int total = 0;
    QJsonArray messages;
    if (!Models::MessageModel::fetchHistory(userId, targetUserId, page, pageSize, &total, &messages)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }

    MessageResult result;
    result.data = QJsonObject {
        { QStringLiteral("total"), total },
        { QStringLiteral("page"), page },
        { QStringLiteral("pageSize"), pageSize },
        { QStringLiteral("list"), messages }
    };
    return result;
}

MessageResult MessageService::sendMessage(const QString& bearerToken,
                                          qint64 receiverId,
                                          const QString& content,
                                          int type) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (receiverId <= 0 || content.trimmed().isEmpty() || content.size() > 5000 || type < 0 || type > 2) {
        return error(400, 1000, QStringLiteral("invalid message payload"));
    }

    bool areFriends = false;
    if (!Models::MessageModel::areFriends(userId, receiverId, &areFriends)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    if (!areFriends) {
        return error(403, 3001, QStringLiteral("receiver is not your friend"));
    }

    qint64 messageId = 0;
    QString createdAt;
    if (!Models::MessageModel::insertMessage(userId, receiverId, content, type, &messageId, &createdAt)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }

    MessageResult result;
    result.data = QJsonObject {
        { QStringLiteral("messageId"), messageId },
        { QStringLiteral("createdAt"), createdAt }
    };
    return result;
}

MessageResult MessageService::error(int httpStatus, int code, const QString& message)
{
    MessageResult result;
    result.httpStatus = httpStatus;
    result.code = code;
    result.message = message;
    return result;
}

bool MessageService::authenticate(const QString& bearerToken, qint64* userId)
{
    const auto trimmed = bearerToken.trimmed();
    if (!trimmed.startsWith(QStringLiteral("Bearer "), Qt::CaseInsensitive)) {
        return false;
    }

    const auto token = trimmed.mid(QStringLiteral("Bearer ").size()).trimmed();
    return Models::MessageModel::findUserIdByToken(token, userId);
}

} // namespace Backend::Services
