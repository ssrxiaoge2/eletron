#pragma once

#include <QJsonObject>
#include <QString>

namespace Backend::Models {

struct FileRecord {
    qint64 id = 0;
    qint64 uploaderId = 0;
    qint64 receiverId = 0;
    qint64 groupId = 0;
    QString fileName;
    QString storedName;
    qint64 fileSize = 0;
    int fileType = 0;
    QString mimeType;
    QString storagePath;
    QString thumbnailPath;
    QString createdAt;
};

class FileModel final {
public:
    static bool insertFile(const FileRecord& file,
                           qint64* fileId,
                           QString* createdAt);
    static bool fetchFile(qint64 fileId, FileRecord* file);
    static bool canAccess(qint64 fileId, qint64 userId, bool* result);
    static QString messageContentJson(const FileRecord& file, qint64 fileId);
    static QJsonObject toJson(const FileRecord& file, bool includeThumbnail);
};

} // namespace Backend::Models
