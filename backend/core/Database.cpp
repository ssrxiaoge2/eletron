#include "Database.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

namespace Backend::Core {

Database& Database::instance()
{
    static Database database;
    return database;
}

bool Database::initialize(const QString& configPath)
{
    if (!Config::instance().load(configPath)) {
        instance().lastErrorText_ = Config::instance().lastErrorText();
        qCritical() << "Failed to load config:" << instance().lastErrorText_;
        return false;
    }

    return instance().open(Config::instance().databaseConfig());
}

QSqlDatabase Database::getConnection()
{
    return instance().connection();
}

bool Database::open(const QString& configPath)
{
    if (!Config::instance().load(configPath)) {
        lastErrorText_ = Config::instance().lastErrorText();
        qCritical() << "Failed to load config:" << lastErrorText_;
        return false;
    }

    return open(Config::instance().databaseConfig());
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
        qCritical() << "Failed to connect MySQL:" << lastErrorText_;
        if (QSqlDatabase::contains(connectionName_)) {
            QSqlDatabase::removeDatabase(connectionName_);
        }
        return false;
    }

    {
        auto db = QSqlDatabase::database(connectionName_, false);
        QSqlQuery query(db);
        if (!query.exec(QStringLiteral("SET NAMES utf8mb4"))) {
            lastErrorText_ = query.lastError().text();
            qCritical() << "Failed to set MySQL charset:" << lastErrorText_;
            close();
            return false;
        }
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

} // namespace Backend::Core
