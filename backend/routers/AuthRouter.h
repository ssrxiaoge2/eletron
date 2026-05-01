#pragma once

#include <QHttpServer>

namespace Backend::Routers {

class AuthRouter final {
public:
    static void registerRoutes(QHttpServer& server);
};

} // namespace Backend::Routers
