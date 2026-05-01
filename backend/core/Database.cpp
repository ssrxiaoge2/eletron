#include "Database.h"

#include <QSettings>
#include <QSqlError>

namespace Backend::Core {

Database& Database::instance()
{
    static Database database;
    return database;
}

bool Database::open(const QString& configPath, const QString& connectionName)
{
    return open(loadConfig(configPath, connectionName));
}

bool Database::open(const DatabaseConfig& config)
{
    close();

    connectionName_ = config.connectionName;
    QString errorText;
    bool opened = false;

    {
        auto db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), connectionName_);
        db.setHostName(config.host);
        db.setPort(config.port);
        db.setDatabaseName(config.databaseName);
        db.setUserName(config.userName);
        db.setPassword(config.password);
        db.setConnectOptions(QStringLiteral("MYSQL_OPT_RECONNECT=1"));

        opened = db.open();
        if (!opened) {
            errorText = db.lastError().text();
        }
    }

    if (!opened) {
        lastErrorText_ = errorText;
        if (QSqlDatabase::contains(connectionName_)) {
            QSqlDatabase::removeDatabase(connectionName_);
        }
        return false;
    }

    lastErrorText_.clear();
    return true;
}

void Database::close()
{
    if (!connectionName_.isEmpty() && QSqlDatabase::contains(connectionName_)) {
        {
            auto db = QSqlDatabase::database(connectionName_, false);
            if (db.isValid() && db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(connectionName_);
    }
}

QSqlDatabase Database::connection() const
{
    return QSqlDatabase::database(connectionName_, false);
}

bool Database::isOpen() const
{
    const auto db = connection();
    return db.isValid() && db.isOpen();
}

QString Database::lastErrorText() const
{
    return lastErrorText_;
}

DatabaseConfig Database::loadConfig(const QString& configPath, const QString& connectionName)
{
    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("mysql"));

    DatabaseConfig config;
    config.host = settings.value(QStringLiteral("host")).toString();
    config.port = settings.value(QStringLiteral("port"), 3306).toInt();
    config.databaseName = settings.value(QStringLiteral("database")).toString();
    config.userName = settings.value(QStringLiteral("username")).toString();
    config.password = settings.value(QStringLiteral("password")).toString();
    config.connectionName = connectionName;
    return config;
}

} // namespace Backend::Core
