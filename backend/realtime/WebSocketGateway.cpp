#include "WebSocketGateway.h"

namespace Backend::Realtime {

WebSocketGateway::WebSocketGateway(QObject* parent)
    : QObject(parent)
    , server_(QStringLiteral("im-backend-ws"), QWebSocketServer::NonSecureMode, this)
{
}

bool WebSocketGateway::listen(const QHostAddress& address, quint16 port)
{
    return server_.listen(address, port);
}

void WebSocketGateway::close()
{
    server_.close();
}

} // namespace Backend::Realtime
