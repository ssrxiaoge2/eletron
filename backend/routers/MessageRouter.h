#pragma once

#include <QHttpServer>

namespace Backend::Routers {

class MessageRouter final {
public:
    static void registerRoutes(QHttpServer& server);
};

} // namespace Backend::Routers
