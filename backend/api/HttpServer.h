#pragma once

#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonObject>
#include <QObject>
#include <QTcpServer>

namespace Backend::Api {

class HttpServer final : public QObject {
public:
    explicit HttpServer(QObject* parent = nullptr);

    bool listen(const QHostAddress& address, quint16 port);

private:
    void registerRoutes();

    QHttpServer server_;
    QTcpServer tcpServer_;
};

} // namespace Backend::Api
