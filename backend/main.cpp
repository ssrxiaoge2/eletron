#include "api/HttpServer.h"
#include "core/Database.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QHostAddress>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("im-backend"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qt/C++ backend service for IM application."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(QStringList() << QStringLiteral("p") << QStringLiteral("port"),
                                  QStringLiteral("HTTP listen port."),
                                  QStringLiteral("port"),
                                  QStringLiteral("8000"));
    QCommandLineOption configOption(QStringList() << QStringLiteral("c") << QStringLiteral("config"),
                                    QStringLiteral("Backend config.ini path."),
                                    QStringLiteral("path"),
                                    QStringLiteral("backend/config/config.ini"));
    parser.addOption(portOption);
    parser.addOption(configOption);
    parser.process(app);

    bool ok = false;
    const auto port = parser.value(portOption).toUShort(&ok);
    if (!ok || port == 0) {
        QTextStream(stderr) << "Invalid port: " << parser.value(portOption) << Qt::endl;
        return 1;
    }

    const auto configPath = parser.value(configOption);
    if (!Backend::Core::Database::initialize(configPath)) {
        QTextStream(stderr) << "Database unavailable: "
                            << Backend::Core::Database::instance().lastErrorText()
                            << Qt::endl;
    }

    Backend::Api::HttpServer server;
    if (!server.listen(QHostAddress::Any, port)) {
        QTextStream(stderr) << "Failed to listen on port " << port << Qt::endl;
        return 1;
    }

    QTextStream(stdout) << "im-backend listening on 0.0.0.0:" << port << Qt::endl;
    return QCoreApplication::exec();
}
