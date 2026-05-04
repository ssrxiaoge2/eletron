#pragma once

#include <QHttpServer>

namespace Backend::Routers {

class GroupRouter final {
public:
    static void registerRoutes(QHttpServer& server);
};

} // namespace Backend::Routers
