#include "MessageRouter.h"

#include "../services/MessageService.h"

#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace Backend::Routers {
namespace {

QString authorizationHeader(const QHttpServerRequest& request)
{
    auto value = request.value(QByteArrayLiteral("Authorization"));
    if (value.isEmpty()) {
        value = request.value(QByteArrayLiteral("authorization"));
    }
    return QString::fromUtf8(value);
}

QHttpServerResponse jsonResponse(const QJsonObject& payload, int statusCode)
{
    return QHttpServerResponse(
        QByteArrayLiteral("application/json"),
        QJsonDocument(payload).toJson(QJsonDocument::Compact),
        static_cast<QHttpServerResponse::StatusCode>(statusCode));
}

QHttpServerResponse serviceResponse(const Services::MessageResult& result)
{
    QJsonObject payload {
        { QStringLiteral("code"), result.code },
        { QStringLiteral("message"), result.message }
    };
    if (!result.data.isUndefined() && !result.data.isNull()) {
        payload.insert(QStringLiteral("data"), result.data);
    }
    return jsonResponse(payload, result.httpStatus);
}

int queryInt(const QUrlQuery& query, const QString& name, int fallback)
{
    bool ok = false;
    const auto value = query.queryItemValue(name).toInt(&ok);
    return ok ? value : fallback;
}

QHttpServerResponse handleReadRequest(const QHttpServerRequest& request)
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(request.body(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return jsonResponse({
                                { QStringLiteral("code"), 1000 },
                                { QStringLiteral("message"), QStringLiteral("invalid request body") }
                            },
                            400);
    }

    const auto targetUserId = document.object().value(QStringLiteral("targetUserId")).toVariant().toLongLong();
    const Services::MessageService service;
    return serviceResponse(service.markRead(authorizationHeader(request), targetUserId));
}

} // namespace

void MessageRouter::registerRoutes(QHttpServer& server)
{
    server.route(QStringLiteral("/api/v1/conversations"),
                 QHttpServerRequest::Method::Get,
                 [](const QHttpServerRequest& request) {
                     const Services::MessageService service;
                     return serviceResponse(service.conversations(authorizationHeader(request)));
                 });

    server.route(QStringLiteral("/api/v1/messages"),
                 QHttpServerRequest::Method::Get,
                 [](const QHttpServerRequest& request) {
                     const auto query = request.query();
                     bool targetOk = false;
                     const auto targetUserId = query.queryItemValue(QStringLiteral("targetUserId")).toLongLong(&targetOk);
                     if (!targetOk) {
                         return jsonResponse({
                                                 { QStringLiteral("code"), 1000 },
                                                 { QStringLiteral("message"), QStringLiteral("targetUserId is required") }
                                             },
                                             400);
                     }

                     const auto page = queryInt(query, QStringLiteral("page"), 1);
                     const auto pageSize = queryInt(query, QStringLiteral("pageSize"), 20);
                     const Services::MessageService service;
                     return serviceResponse(service.history(
                         authorizationHeader(request),
                         targetUserId,
                         page,
                         pageSize));
                 });

    server.route(QStringLiteral("/api/v1/messages"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     QJsonParseError parseError;
                     const auto document = QJsonDocument::fromJson(request.body(), &parseError);
                     if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                         return jsonResponse({
                                                 { QStringLiteral("code"), 1000 },
                                                 { QStringLiteral("message"), QStringLiteral("invalid request body") }
                                             },
                                             400);
                     }

                     const auto body = document.object();
                     const auto receiverId = body.value(QStringLiteral("receiverId")).toVariant().toLongLong();
                     const auto content = body.value(QStringLiteral("content")).toString();
                     const auto type = body.value(QStringLiteral("type")).toInt(0);

                     const Services::MessageService service;
                     return serviceResponse(service.sendMessage(
                         authorizationHeader(request),
                         receiverId,
                         content,
                         type));
                 });

    server.route(QStringLiteral("/api/v1/messages/read"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     return handleReadRequest(request);
                 });

    server.route(QStringLiteral("/api/v1/conversations/read"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     return handleReadRequest(request);
                 });
}

} // namespace Backend::Routers
