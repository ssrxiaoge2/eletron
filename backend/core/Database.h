#pragma once

#include <QSqlDatabase>
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

class Database final {
public:
    static Database& instance();

    bool open(const QString& configPath, const QString& connectionName = QStringLiteral("backend_mysql"));
    bool open(const DatabaseConfig& config);
    void close();

    QSqlDatabase connection() const;
    bool isOpen() const;
    QString lastErrorText() const;

private:
    Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    static DatabaseConfig loadConfig(const QString& configPath, const QString& connectionName);

    QString connectionName_ = QStringLiteral("backend_mysql");
    QString lastErrorText_;
};

} // namespace Backend::Core
