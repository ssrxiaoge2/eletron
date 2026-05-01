#include "Config.h"

#include <QFileInfo>
#include <QSettings>

namespace Backend::Core {

Config& Config::instance()
{
    static Config config;
    return config;
}

bool Config::load(const QString& path)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        lastErrorText_ = QStringLiteral("Config file does not exist: %1").arg(path);
        return false;
    }

    QSettings settings(path, QSettings::IniFormat);

    settings.beginGroup(QStringLiteral("mysql"));
    databaseConfig_.host = settings.value(QStringLiteral("host")).toString().trimmed();
    databaseConfig_.port = settings.value(QStringLiteral("port"), 3306).toInt();
    databaseConfig_.databaseName = settings.value(QStringLiteral("database")).toString().trimmed();
    databaseConfig_.userName = settings.value(QStringLiteral("username")).toString().trimmed();
    databaseConfig_.password = settings.value(QStringLiteral("password")).toString();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("jwt"));
    jwtSecret_ = settings.value(QStringLiteral("secret")).toString();
    settings.endGroup();

    if (databaseConfig_.host.isEmpty()
        || databaseConfig_.databaseName.isEmpty()
        || databaseConfig_.userName.isEmpty()) {
        lastErrorText_ = QStringLiteral("MySQL host, database, and username are required.");
        return false;
    }

    if (jwtSecret_.isEmpty()) {
        lastErrorText_ = QStringLiteral("JWT secret is required.");
        return false;
    }

    lastErrorText_.clear();
    return true;
}

DatabaseConfig Config::databaseConfig() const
{
    return databaseConfig_;
}

QString Config::jwtSecret() const
{
    return jwtSecret_;
}

QString Config::lastErrorText() const
{
    return lastErrorText_;
}

} // namespace Backend::Core
