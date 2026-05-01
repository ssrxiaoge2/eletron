#pragma once

#include <QString>

namespace Backend::Core {

struct DatabaseConfig {
    QString host;
    int port = 3306;
    QString databaseName;
    QString userName;
    QString password;
    QString connectionName = QStringLiteral("backend_mysql");
};

class Config final {
public:
    static Config& instance();

    bool load(const QString& path);

    DatabaseConfig databaseConfig() const;
    QString jwtSecret() const;
    QString lastErrorText() const;

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    DatabaseConfig databaseConfig_;
    QString jwtSecret_;
    QString lastErrorText_;
};

} // namespace Backend::Core
