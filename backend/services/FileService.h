#pragma once

#include <QByteArray>
#include <QJsonValue>
#include <QString>

namespace Backend::Services {

struct UploadedFilePart {
    QString fileName;
    QString mimeType;
    QByteArray bytes;
};

struct FileResult {
    int httpStatus = 200;
    int code = 0;
    QString message = QStringLiteral("ok");
    QJsonValue data;
};

struct FileBinaryResult {
    int httpStatus = 200;
    int code = 0;
    QString message = QStringLiteral("ok");
    QByteArray data;
    QString mimeType;
    QString fileName;
    qint64 fileSize = 0;
};

class FileService final {
public:
    FileResult upload(const QString& bearerToken,
                      const UploadedFilePart& file,
                      int type,
                      qint64 receiverId) const;
    FileBinaryResult download(const QString& bearerToken, qint64 fileId) const;
    FileBinaryResult thumbnail(const QString& bearerToken, qint64 fileId) const;
    FileResult info(const QString& bearerToken, qint64 fileId) const;

private:
    static FileResult error(int httpStatus, int code, const QString& message);
    static FileBinaryResult binaryError(int httpStatus, int code, const QString& message);
    static bool authenticate(const QString& bearerToken, qint64* userId);
};

} // namespace Backend::Services
