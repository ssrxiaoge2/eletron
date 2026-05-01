#pragma once

#include <QHttpServer>

namespace Backend::Routers {

class UserRouter final {
public:
    static void registerRoutes(QHttpServer& server);
};

} // namespace Backend::Routers
