#pragma once

#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>

struct TestUser {
    qint64 userId = 0;
    QString username;
    QString password;
    QString token;
};

TestUser registerAndLogin(ApiClient& client, const QString& username, const QString& password = QStringLiteral("123456"));
QMap<QString, TestUser> createDefaultUsers(ApiClient& client);
bool makeFriends(ApiClient& client, const TestUser& from, const TestUser& to);
qint64 createGroup(ApiClient& client, const TestUser& owner, const QList<TestUser>& members, const QString& name = QStringLiteral("测试群"));
bool arrayContainsUserId(const QJsonArray& array, qint64 userId);
qint64 firstRequestIdFrom(const QJsonArray& array, qint64 fromUserId);
QJsonObject findObjectByUserId(const QJsonArray& array, qint64 userId);
QJsonObject findGroupByName(const QJsonArray& array, const QString& name);
