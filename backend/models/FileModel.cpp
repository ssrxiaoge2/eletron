#include "FileModel.h"

#include "../core/Database.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QVariant>

namespace Backend::Models {
namespace {

QString dateTimeExpression(const QString& column)
{
    return QStringLiteral("DATE_FORMAT(%1, '%Y-%m-%d %H:%i:%s')").arg(column);
}

QString downloadUrl(qint64 fileId)
{
    return QStringLiteral("/api/v1/files/download/%1").arg(fileId);
}

QString thumbnailUrl(qint64 fileId, const QString& thumbnailPath)
{
    return thumbnailPath.isEmpty() ? QString() : QStringLiteral("/api/v1/files/thumbnail/%1").arg(fileId);
}

} // namespace

bool FileModel::insertFileWithMessage(const FileRecord& file,
                                      qint64* fileId,
                                      QString* createdAt)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }
    if (!db.transaction()) {
        return false;
    }

    QSqlQuery fileQuery(db);
    fileQuery.prepare(QStringLiteral(
        "INSERT INTO files "
        "(uploader_id, receiver_id, file_name, stored_name, file_size, file_type, mime_type, storage_path, thumbnail_path) "
        "VALUES (:uploader_id, :receiver_id, :file_name, :stored_name, :file_size, :file_type, :mime_type, :storage_path, :thumbnail_path)"));
    fileQuery.bindValue(QStringLiteral(":uploader_id"), file.uploaderId);
    fileQuery.bindValue(QStringLiteral(":receiver_id"), file.receiverId);
    fileQuery.bindValue(QStringLiteral(":file_name"), file.fileName);
    fileQuery.bindValue(QStringLiteral(":stored_name"), file.storedName);
    fileQuery.bindValue(QStringLiteral(":file_size"), file.fileSize);
    fileQuery.bindValue(QStringLiteral(":file_type"), file.fileType);
    fileQuery.bindValue(QStringLiteral(":mime_type"), file.mimeType);
    fileQuery.bindValue(QStringLiteral(":storage_path"), file.storagePath);
    fileQuery.bindValue(QStringLiteral(":thumbnail_path"), file.thumbnailPath);
    if (!fileQuery.exec()) {
        db.rollback();
        return false;
    }

    *fileId = fileQuery.lastInsertId().toLongLong();

    const QJsonObject messageContent {
        { QStringLiteral("fileId"), *fileId },
        { QStringLiteral("fileName"), file.fileName },
        { QStringLiteral("fileSize"), file.fileSize },
        { QStringLiteral("url"), downloadUrl(*fileId) },
        { QStringLiteral("thumbnailUrl"), thumbnailUrl(*fileId, file.thumbnailPath) }
    };

    QSqlQuery messageQuery(db);
    messageQuery.prepare(QStringLiteral(
        "INSERT INTO messages (sender_id, receiver_id, content, type) "
        "VALUES (:sender_id, :receiver_id, :content, :type)"));
    messageQuery.bindValue(QStringLiteral(":sender_id"), file.uploaderId);
    messageQuery.bindValue(QStringLiteral(":receiver_id"), file.receiverId);
    messageQuery.bindValue(QStringLiteral(":content"),
                           QString::fromUtf8(QJsonDocument(messageContent).toJson(QJsonDocument::Compact)));
    messageQuery.bindValue(QStringLiteral(":type"), file.fileType);
    if (!messageQuery.exec()) {
        db.rollback();
        return false;
    }

    QSqlQuery selectQuery(db);
    selectQuery.prepare(QStringLiteral(
        "SELECT %1 AS created_at FROM files WHERE id = :id LIMIT 1")
                            .arg(dateTimeExpression(QStringLiteral("created_at"))));
    selectQuery.bindValue(QStringLiteral(":id"), *fileId);
    if (!selectQuery.exec() || !selectQuery.next()) {
        db.rollback();
        return false;
    }
    *createdAt = selectQuery.value(QStringLiteral("created_at")).toString();

    if (!db.commit()) {
        db.rollback();
        return false;
    }

    return true;
}

bool FileModel::fetchFile(qint64 fileId, FileRecord* file)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, uploader_id, receiver_id, file_name, stored_name, file_size, file_type, "
        "mime_type, storage_path, thumbnail_path, %1 AS created_at "
        "FROM files WHERE id = :id AND is_deleted = 0 LIMIT 1")
                      .arg(dateTimeExpression(QStringLiteral("created_at"))));
    query.bindValue(QStringLiteral(":id"), fileId);
    if (!query.exec() || !query.next()) {
        return false;
    }

    file->id = query.value(QStringLiteral("id")).toLongLong();
    file->uploaderId = query.value(QStringLiteral("uploader_id")).toLongLong();
    file->receiverId = query.value(QStringLiteral("receiver_id")).toLongLong();
    file->fileName = query.value(QStringLiteral("file_name")).toString();
    file->storedName = query.value(QStringLiteral("stored_name")).toString();
    file->fileSize = query.value(QStringLiteral("file_size")).toLongLong();
    file->fileType = query.value(QStringLiteral("file_type")).toInt();
    file->mimeType = query.value(QStringLiteral("mime_type")).toString();
    file->storagePath = query.value(QStringLiteral("storage_path")).toString();
    file->thumbnailPath = query.value(QStringLiteral("thumbnail_path")).toString();
    file->createdAt = query.value(QStringLiteral("created_at")).toString();
    return true;
}

bool FileModel::canAccess(qint64 fileId, qint64 userId, bool* result)
{
    FileRecord file;
    if (!fetchFile(fileId, &file)) {
        return false;
    }

    *result = file.uploaderId == userId || file.receiverId == userId;
    return true;
}

QJsonObject FileModel::toJson(const FileRecord& file, bool includeThumbnail)
{
    QJsonObject data {
        { QStringLiteral("fileId"), file.id },
        { QStringLiteral("fileName"), file.fileName },
        { QStringLiteral("fileSize"), file.fileSize },
        { QStringLiteral("fileType"), file.fileType },
        { QStringLiteral("mimeType"), file.mimeType },
        { QStringLiteral("uploaderUserId"), file.uploaderId },
        { QStringLiteral("url"), downloadUrl(file.id) },
        { QStringLiteral("createdAt"), file.createdAt }
    };
    if (includeThumbnail) {
        data.insert(QStringLiteral("thumbnailUrl"), thumbnailUrl(file.id, file.thumbnailPath));
    }
    return data;
}

} // namespace Backend::Models
