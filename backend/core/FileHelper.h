#pragma once

#include <QByteArray>
#include <QString>

namespace Backend::Core {

class FileHelper final {
public:
    static QString storageRoot();
    static QString filesDir();
    static QString imagesDir();
    static QString thumbnailsDir();
    static bool ensureStorageDirs();
    static QString sanitizedFileName(const QString& fileName);
    static QString uniqueStoredName(const QString& originalFileName);
    static bool saveBytes(const QString& path, const QByteArray& bytes);
    static bool createThumbnail(const QString& sourcePath, const QString& thumbnailPath);
};

} // namespace Backend::Core
