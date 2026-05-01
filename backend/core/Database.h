#pragma once

#include "Config.h"

#include <QSqlDatabase>
#include <QString>

namespace Backend::Core {

class Database final {
public:
    static Database& instance();

    static bool initialize(const QString& configPath);
    static QSqlDatabase getConnection();

    bool open(const QString& configPath);
    bool open(const DatabaseConfig& config);
    void close();

    QSqlDatabase connection() const;
    bool isOpen() const;
    QString lastErrorText() const;

private:
    Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QString connectionName_ = QStringLiteral("backend_mysql");
    QString lastErrorText_;
};

} // namespace Backend::Core
