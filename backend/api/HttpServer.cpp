#include "HttpServer.h"

#include "../routers/AuthRouter.h"
#include "../routers/FriendRouter.h"
#include "../routers/MessageRouter.h"
#include "../routers/UserRouter.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

namespace Backend::Api {

HttpServer::HttpServer(QObject* parent)
    : QObject(parent)
{
    registerRoutes();
}

bool HttpServer::listen(const QHostAddress& address, quint16 port)
{
    if (!tcpServer_.listen(address, port)) {
        return false;
    }

    if (!server_.bind(&tcpServer_)) {
        tcpServer_.close();
        return false;
    }

    return true;
}

void HttpServer::registerRoutes()
{
    server_.route(QStringLiteral("/health"), [] {
        const QJsonObject payload {
            { QStringLiteral("status"), QStringLiteral("ok") },
            { QStringLiteral("service"), QStringLiteral("im-backend") },
            { QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate) }
        };

        return QHttpServerResponse(
            QByteArrayLiteral("application/json"),
            QJsonDocument(payload).toJson(QJsonDocument::Compact));
    });

    Backend::Routers::AuthRouter::registerRoutes(server_);
    Backend::Routers::MessageRouter::registerRoutes(server_);
    Backend::Routers::FriendRouter::registerRoutes(server_);
    Backend::Routers::UserRouter::registerRoutes(server_);
}

} // namespace Backend::Api
