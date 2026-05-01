#pragma once

#include <QJsonObject>
#include <QString>

namespace Backend::Models {

struct UserModel {
    qint64 id = 0;
    QString username;
    QString nickname;
    QString passwordHash;
    QString avatar;
    QString signature;
    QString email;
    QString gender;
    QString birthday;
    int status = 0;
};

class UserRepository final {
public:
    static bool fetchProfile(qint64 userId, QJsonObject* profile);
    static bool updateProfile(qint64 userId, const QJsonObject& fields, QJsonObject* updatedProfile);
};

} // namespace Backend::Models
