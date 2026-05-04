#include "ChatInputEdit.h"

#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtGui/QClipboard>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

namespace {

bool IsImagePath(const QString &path) {
  const QString suffix = QFileInfo(path).suffix().toLower();
  return suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
         suffix == "gif" || suffix == "bmp" || suffix == "webp";
}

}  // namespace

ChatInputEdit::ChatInputEdit(QWidget *parent) : QTextEdit(parent) {
  setAcceptDrops(true);
}

void ChatInputEdit::keyPressEvent(QKeyEvent *event) {
  if (event->matches(QKeySequence::Paste)) {
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mime = clipboard->mimeData();
    if (mime->hasImage()) {
      QPixmap pixmap = qvariant_cast<QPixmap>(mime->imageData());
      if (!pixmap.isNull()) {
        emit imagePasted(pixmap);
        return;
      }
    }
  }
  QTextEdit::keyPressEvent(event);
}

void ChatInputEdit::dragEnterEvent(QDragEnterEvent *event) {
  const QMimeData *mime = event->mimeData();
  if (mime->hasImage()) {
    event->acceptProposedAction();
    return;
  }
  if (mime->hasUrls()) {
    for (const QUrl &url : mime->urls()) {
      if (IsImagePath(url.toLocalFile())) {
        event->acceptProposedAction();
        return;
      }
    }
  }
  QTextEdit::dragEnterEvent(event);
}

void ChatInputEdit::dropEvent(QDropEvent *event) {
  const QMimeData *mime = event->mimeData();
  if (mime->hasUrls()) {
    bool handled = false;
    for (const QUrl &url : mime->urls()) {
      const QString path = url.toLocalFile();
      if (IsImagePath(path)) {
        emit imageDropped(path);
        handled = true;
      }
    }
    if (handled) {
      event->acceptProposedAction();
      return;
    }
  }
  QTextEdit::dropEvent(event);
}
