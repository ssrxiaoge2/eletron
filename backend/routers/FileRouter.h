#pragma once

#include <QHttpServer>

namespace Backend::Routers {

class FileRouter final {
public:
    static void registerRoutes(QHttpServer& server);
};

} // namespace Backend::Routers
