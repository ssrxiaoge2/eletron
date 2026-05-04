#include "FileService.h"

#include "../core/FileHelper.h"
#include "../models/FileModel.h"
#include "../models/MessageModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QMimeDatabase>

namespace Backend::Services {
namespace {

constexpr qint64 MaxImageSize = 20LL * 1024 * 1024;
constexpr qint64 MaxFileSize = 100LL * 1024 * 1024;

QString tokenFromBearer(const QString& bearerToken)
{
    const auto trimmed = bearerToken.trimmed();
    if (!trimmed.startsWith(QStringLiteral("Bearer "), Qt::CaseInsensitive)) {
        return {};
    }
    return trimmed.mid(QStringLiteral("Bearer ").size()).trimmed();
}

QString thumbnailUrl(qint64 fileId, int type)
{
    return type == 1 ? QStringLiteral("/api/v1/files/thumbnail/%1").arg(fileId) : QString();
}

} // namespace

FileResult FileService::upload(const QString& bearerToken,
                               const UploadedFilePart& file,
                               int type,
                               qint64 receiverId) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (receiverId <= 0 || receiverId == userId || file.bytes.isEmpty() || file.fileName.trimmed().isEmpty()) {
        return error(400, 1000, QStringLiteral("invalid upload payload"));
    }
    if (type != 1 && type != 2) {
        return error(400, 4003, QStringLiteral("unsupported file type"));
    }

    bool areFriends = false;
    if (!Models::MessageModel::areFriends(userId, receiverId, &areFriends)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    if (!areFriends) {
        return error(403, 3001, QStringLiteral("receiver is not your friend"));
    }

    const auto fileSize = file.bytes.size();
    if ((type == 1 && fileSize > MaxImageSize) || (type == 2 && fileSize > MaxFileSize)) {
        return error(413, 4001, QStringLiteral("file exceeds size limit"));
    }
    if (!Core::FileHelper::ensureStorageDirs()) {
        return error(503, 2002, QStringLiteral("file_storage_unavailable"));
    }

    auto mimeType = file.mimeType.trimmed();
    if (mimeType.isEmpty()) {
        mimeType = QMimeDatabase().mimeTypeForFile(file.fileName).name();
    }
    if (type == 1 && !mimeType.startsWith(QStringLiteral("image/"))) {
        return error(400, 4003, QStringLiteral("unsupported file type"));
    }

    const auto originalName = Core::FileHelper::sanitizedFileName(file.fileName);
    const auto storedName = Core::FileHelper::uniqueStoredName(originalName);
    const auto storageDir = type == 1 ? Core::FileHelper::imagesDir() : Core::FileHelper::filesDir();
    const auto storagePath = QDir(storageDir).filePath(storedName);
    if (!Core::FileHelper::saveBytes(storagePath, file.bytes)) {
        return error(503, 2002, QStringLiteral("file_storage_unavailable"));
    }

    QString thumbnailPath;
    if (type == 1) {
        thumbnailPath = QDir(Core::FileHelper::thumbnailsDir()).filePath(storedName + QStringLiteral(".png"));
        if (!Core::FileHelper::createThumbnail(storagePath, thumbnailPath)) {
            QFile::remove(storagePath);
            return error(400, 4003, QStringLiteral("unsupported file type"));
        }
    }

    Models::FileRecord record;
    record.uploaderId = userId;
    record.receiverId = receiverId;
    record.fileName = originalName;
    record.storedName = storedName;
    record.fileSize = fileSize;
    record.fileType = type;
    record.mimeType = mimeType;
    record.storagePath = QFileInfo(storagePath).absoluteFilePath();
    record.thumbnailPath = thumbnailPath.isEmpty() ? QString() : QFileInfo(thumbnailPath).absoluteFilePath();

    qint64 fileId = 0;
    QString createdAt;
    if (!Models::FileModel::insertFileWithMessage(record, &fileId, &createdAt)) {
        QFile::remove(record.storagePath);
        if (!record.thumbnailPath.isEmpty()) {
            QFile::remove(record.thumbnailPath);
        }
        return error(503, 2002, QStringLiteral("database_error"));
    }

    Models::FileRecord saved = record;
    saved.id = fileId;
    saved.createdAt = createdAt;
    auto data = Models::FileModel::toJson(saved, true);
    data.insert(QStringLiteral("thumbnailUrl"), thumbnailUrl(fileId, type));

    FileResult result;
    result.data = data;
    return result;
}

FileBinaryResult FileService::download(const QString& bearerToken, qint64 fileId) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return binaryError(401, 1002, QStringLiteral("invalid or expired token"));
    }

    Models::FileRecord file;
    if (!Models::FileModel::fetchFile(fileId, &file)) {
        return binaryError(404, 4002, QStringLiteral("file not found"));
    }
    if (file.uploaderId != userId && file.receiverId != userId) {
        return binaryError(403, 1003, QStringLiteral("无权访问"));
    }

    QFile diskFile(file.storagePath);
    if (!diskFile.open(QIODevice::ReadOnly)) {
        return binaryError(404, 4002, QStringLiteral("file not found"));
    }

    FileBinaryResult result;
    result.data = diskFile.readAll();
    result.mimeType = QStringLiteral("application/octet-stream");
    result.fileName = file.fileName;
    result.fileSize = result.data.size();
    return result;
}

FileBinaryResult FileService::thumbnail(const QString& bearerToken, qint64 fileId) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return binaryError(401, 1002, QStringLiteral("invalid or expired token"));
    }

    Models::FileRecord file;
    if (!Models::FileModel::fetchFile(fileId, &file) || file.fileType != 1 || file.thumbnailPath.isEmpty()) {
        return binaryError(404, 4002, QStringLiteral("file not found"));
    }
    if (file.uploaderId != userId && file.receiverId != userId) {
        return binaryError(403, 1003, QStringLiteral("无权访问"));
    }

    QFile diskFile(file.thumbnailPath);
    if (!diskFile.open(QIODevice::ReadOnly)) {
        return binaryError(404, 4002, QStringLiteral("file not found"));
    }

    FileBinaryResult result;
    result.data = diskFile.readAll();
    result.mimeType = QStringLiteral("image/png");
    result.fileName = file.fileName + QStringLiteral(".thumbnail.png");
    result.fileSize = result.data.size();
    return result;
}

FileResult FileService::info(const QString& bearerToken, qint64 fileId) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }

    Models::FileRecord file;
    if (!Models::FileModel::fetchFile(fileId, &file)) {
        return error(404, 4002, QStringLiteral("file not found"));
    }
    if (file.uploaderId != userId && file.receiverId != userId) {
        return error(403, 1003, QStringLiteral("无权访问"));
    }

    FileResult result;
    result.data = Models::FileModel::toJson(file, false);
    return result;
}

FileResult FileService::error(int httpStatus, int code, const QString& message)
{
    FileResult result;
    result.httpStatus = httpStatus;
    result.code = code;
    result.message = message;
    return result;
}

FileBinaryResult FileService::binaryError(int httpStatus, int code, const QString& message)
{
    FileBinaryResult result;
    result.httpStatus = httpStatus;
    result.code = code;
    result.message = message;
    return result;
}

bool FileService::authenticate(const QString& bearerToken, qint64* userId)
{
    return Models::MessageModel::findUserIdByToken(tokenFromBearer(bearerToken), userId);
}

} // namespace Backend::Services
