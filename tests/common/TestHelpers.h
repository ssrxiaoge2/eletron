#pragma once

#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

struct TestUser {
    qint64 userId = 0;
    QString username;
    QString password;
    QString token;
};

TestUser registerAndLogin(ApiClient& client, const QString& username, const QString& password = QStringLiteral("123456"));
bool makeFriends(ApiClient& client, const TestUser& from, const TestUser& to);
bool arrayContainsUserId(const QJsonArray& array, qint64 userId);
qint64 firstRequestIdFrom(const QJsonArray& array, qint64 fromUserId);
