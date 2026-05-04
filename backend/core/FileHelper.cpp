#include "FileHelper.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QUuid>

namespace Backend::Core {

QString FileHelper::storageRoot()
{
    return QDir::current().filePath(QStringLiteral("backend/storage"));
}

QString FileHelper::filesDir()
{
    return QDir(storageRoot()).filePath(QStringLiteral("files"));
}

QString FileHelper::imagesDir()
{
    return QDir(storageRoot()).filePath(QStringLiteral("images"));
}

QString FileHelper::thumbnailsDir()
{
    return QDir(storageRoot()).filePath(QStringLiteral("thumbnails"));
}

bool FileHelper::ensureStorageDirs()
{
    QDir dir;
    return dir.mkpath(filesDir())
        && dir.mkpath(imagesDir())
        && dir.mkpath(thumbnailsDir());
}

QString FileHelper::sanitizedFileName(const QString& fileName)
{
    auto clean = QFileInfo(fileName).fileName().trimmed();
    clean.replace(QLatin1Char('\\'), QLatin1Char('_'));
    clean.replace(QLatin1Char('/'), QLatin1Char('_'));
    clean.replace(QLatin1Char(':'), QLatin1Char('_'));
    clean.replace(QLatin1Char('*'), QLatin1Char('_'));
    clean.replace(QLatin1Char('?'), QLatin1Char('_'));
    clean.replace(QLatin1Char('"'), QLatin1Char('_'));
    clean.replace(QLatin1Char('<'), QLatin1Char('_'));
    clean.replace(QLatin1Char('>'), QLatin1Char('_'));
    clean.replace(QLatin1Char('|'), QLatin1Char('_'));
    return clean.isEmpty() ? QStringLiteral("upload.bin") : clean.left(256);
}

QString FileHelper::uniqueStoredName(const QString& originalFileName)
{
    return QStringLiteral("%1_%2")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces), sanitizedFileName(originalFileName));
}

bool FileHelper::saveBytes(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    return file.write(bytes) == bytes.size();
}

bool FileHelper::createThumbnail(const QString& sourcePath, const QString& thumbnailPath)
{
    QImage image(sourcePath);
    if (image.isNull()) {
        return false;
    }

    const auto thumbnail = image.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return thumbnail.save(thumbnailPath, "PNG");
}

} // namespace Backend::Core
