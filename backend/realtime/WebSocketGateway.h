#pragma once

#include <QHostAddress>
#include <QObject>
#include <QWebSocketServer>

namespace Backend::Realtime {

class WebSocketGateway final : public QObject {
public:
    explicit WebSocketGateway(QObject* parent = nullptr);

    bool listen(const QHostAddress& address, quint16 port);
    void close();

private:
    QWebSocketServer server_;
};

} // namespace Backend::Realtime
