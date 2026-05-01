#include "UserModel.h"

#include "../core/Database.h"

#include <QJsonObject>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace Backend::Models {

namespace {

QString formatDate(const QString& column)
{
    return QStringLiteral("DATE_FORMAT(%1, '%Y-%m-%d')").arg(column);
}

} // namespace

bool UserRepository::fetchProfile(qint64 userId, QJsonObject* profile)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, username, nickname, status, avatar, gender, "
        "DATE_FORMAT(birthday, '%Y-%m-%d') AS birthday, signature, email, %1 AS created_at "
        "FROM users "
        "WHERE id = :user_id AND is_deleted = 0 LIMIT 1").arg(formatDate(QStringLiteral("created_at"))));
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec() || !query.next()) {
        return false;
    }

    *profile = QJsonObject {
        { QStringLiteral("userId"), query.value(QStringLiteral("id")).toLongLong() },
        { QStringLiteral("username"), query.value(QStringLiteral("username")).toString() },
        { QStringLiteral("nickname"), query.value(QStringLiteral("nickname")).toString() },
        { QStringLiteral("status"), query.value(QStringLiteral("status")).toInt() },
        { QStringLiteral("avatar"), query.value(QStringLiteral("avatar")).toString() },
        { QStringLiteral("gender"), query.value(QStringLiteral("gender")).toString() },
        { QStringLiteral("birthday"), query.value(QStringLiteral("birthday")).toString() },
        { QStringLiteral("signature"), query.value(QStringLiteral("signature")).toString() },
        { QStringLiteral("email"), query.value(QStringLiteral("email")).toString() },
        { QStringLiteral("createdAt"), query.value(QStringLiteral("created_at")).toString() }
    };
    return true;
}

bool UserRepository::updateProfile(qint64 userId, const QJsonObject& fields, QJsonObject* updatedProfile)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QStringList assignments;
    if (fields.contains(QStringLiteral("nickname"))) {
        assignments << QStringLiteral("nickname = :nickname");
    }
    if (fields.contains(QStringLiteral("signature"))) {
        assignments << QStringLiteral("signature = :signature");
    }
    if (fields.contains(QStringLiteral("avatar"))) {
        assignments << QStringLiteral("avatar = :avatar");
    }
    if (fields.contains(QStringLiteral("gender"))) {
        assignments << QStringLiteral("gender = :gender");
    }
    if (fields.contains(QStringLiteral("birthday"))) {
        assignments << QStringLiteral("birthday = :birthday");
    }
    if (fields.contains(QStringLiteral("email"))) {
        assignments << QStringLiteral("email = :email");
    }

    if (!assignments.isEmpty()) {
        QSqlQuery updateQuery(db);
        updateQuery.prepare(QStringLiteral("UPDATE users SET %1 WHERE id = :user_id AND is_deleted = 0")
                                .arg(assignments.join(QStringLiteral(", "))));
        updateQuery.bindValue(QStringLiteral(":user_id"), userId);
        if (fields.contains(QStringLiteral("nickname"))) {
            updateQuery.bindValue(QStringLiteral(":nickname"), fields.value(QStringLiteral("nickname")).toString());
        }
        if (fields.contains(QStringLiteral("signature"))) {
            updateQuery.bindValue(QStringLiteral(":signature"), fields.value(QStringLiteral("signature")).toString());
        }
        if (fields.contains(QStringLiteral("avatar"))) {
            updateQuery.bindValue(QStringLiteral(":avatar"), fields.value(QStringLiteral("avatar")).toString());
        }
        if (fields.contains(QStringLiteral("gender"))) {
            updateQuery.bindValue(QStringLiteral(":gender"), fields.value(QStringLiteral("gender")).toString());
        }
        if (fields.contains(QStringLiteral("birthday"))) {
            const auto birthday = fields.value(QStringLiteral("birthday")).toString().trimmed();
            updateQuery.bindValue(QStringLiteral(":birthday"), birthday.isEmpty() ? QVariant() : QVariant(birthday));
        }
        if (fields.contains(QStringLiteral("email"))) {
            updateQuery.bindValue(QStringLiteral(":email"), fields.value(QStringLiteral("email")).toString());
        }
        if (!updateQuery.exec()) {
            return false;
        }
    }

    QSqlQuery selectQuery(db);
    selectQuery.prepare(QStringLiteral(
        "SELECT nickname, signature, avatar, gender, DATE_FORMAT(birthday, '%Y-%m-%d') AS birthday, email FROM users "
        "WHERE id = :user_id AND is_deleted = 0 LIMIT 1"));
    selectQuery.bindValue(QStringLiteral(":user_id"), userId);
    if (!selectQuery.exec() || !selectQuery.next()) {
        return false;
    }

    *updatedProfile = QJsonObject {
        { QStringLiteral("nickname"), selectQuery.value(QStringLiteral("nickname")).toString() },
        { QStringLiteral("signature"), selectQuery.value(QStringLiteral("signature")).toString() },
        { QStringLiteral("avatar"), selectQuery.value(QStringLiteral("avatar")).toString() },
        { QStringLiteral("gender"), selectQuery.value(QStringLiteral("gender")).toString() },
        { QStringLiteral("birthday"), selectQuery.value(QStringLiteral("birthday")).toString() },
        { QStringLiteral("email"), selectQuery.value(QStringLiteral("email")).toString() }
    };
    return true;
}

} // namespace Backend::Models
