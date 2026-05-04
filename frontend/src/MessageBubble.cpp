#include "MessageBubble.h"

#include "ApiClient.h"

#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QtGlobal>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPixmap>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>

namespace {

constexpr int kPreviewSize = 200;

QJsonObject ParseContentJson(const QString &content) {
  const QJsonDocument document = QJsonDocument::fromJson(content.toUtf8());
  return document.isObject() ? document.object() : QJsonObject();
}

QString ReadableFileSize(qint64 size) {
  if (size < 1024) {
    return QStringLiteral("%1 B").arg(size);
  }
  if (size < 1024 * 1024) {
    return QStringLiteral("%1 KB").arg(size / 1024.0, 0, 'f', 1);
  }
  if (size < 1024LL * 1024 * 1024) {
    return QStringLiteral("%1 MB").arg(size / 1024.0 / 1024.0, 0, 'f', 1);
  }
  return QStringLiteral("%1 GB").arg(size / 1024.0 / 1024.0 / 1024.0, 0, 'f',
                                    1);
}

QString FileIconText(const QString &file_name) {
  const QString suffix = QFileInfo(file_name).suffix().toLower();
  if (suffix == "pdf") {
    return QStringLiteral("PDF");
  }
  if (suffix == "doc" || suffix == "docx") {
    return QStringLiteral("DOC");
  }
  if (suffix == "xls" || suffix == "xlsx") {
    return QStringLiteral("XLS");
  }
  if (suffix == "zip" || suffix == "rar") {
    return QStringLiteral("ZIP");
  }
  return QStringLiteral("FILE");
}

QString FileIconColor(const QString &file_name) {
  const QString suffix = QFileInfo(file_name).suffix().toLower();
  if (suffix == "pdf") {
    return QStringLiteral("#d9534f");
  }
  if (suffix == "doc" || suffix == "docx") {
    return QStringLiteral("#2f80ed");
  }
  if (suffix == "xls" || suffix == "xlsx") {
    return QStringLiteral("#27ae60");
  }
  if (suffix == "zip" || suffix == "rar") {
    return QStringLiteral("#f2c94c");
  }
  return QStringLiteral("#8f9ba8");
}

QPixmap RoundedPixmap(const QPixmap &source, const QSize &max_size,
                      int radius) {
  const QPixmap scaled =
      source.scaled(max_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  QPixmap rounded(scaled.size());
  rounded.fill(Qt::transparent);

  QPainter painter(&rounded);
  painter.setRenderHint(QPainter::Antialiasing);
  QPainterPath path;
  path.addRoundedRect(rounded.rect(), radius, radius);
  painter.setClipPath(path);
  painter.drawPixmap(0, 0, scaled);
  return rounded;
}

}  // namespace

MessageBubble::MessageBubble(const QString &content, BubbleType type,
                             bool is_self, const QString &time, QWidget *parent)
    : QWidget(parent), type_(type), is_self_(is_self) {
  Q_UNUSED(time);

  auto *root_layout = new QHBoxLayout(this);
  auto *avatar = new QLabel(is_self ? QStringLiteral("\u6211")
                                    : QStringLiteral("\u53cb"),
                            this);
  QWidget *content_widget = CreateContentWidget(content);

  setObjectName(is_self ? "sentMessageRow" : "receivedMessageRow");
  setAttribute(Qt::WA_TranslucentBackground, true);
  setStyleSheet(QStringLiteral("QWidget#%1 { background: transparent; }")
                    .arg(objectName()));
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  avatar->setObjectName("messageAvatar");
  avatar->setAlignment(Qt::AlignCenter);
  avatar->setFixedSize(36, 36);

  root_layout->setContentsMargins(18, 6, 18, 6);
  root_layout->setSpacing(8);

  if (is_self) {
    root_layout->addStretch();
    root_layout->addWidget(content_widget);
    root_layout->addWidget(avatar);
  } else {
    root_layout->addWidget(avatar);
    root_layout->addWidget(content_widget);
    root_layout->addStretch();
  }
}

void MessageBubble::setMaxBubbleWidth(int max_width) {
  if (bubble_ != nullptr) {
    bubble_->setMaximumWidth(max_width);
  }
}

bool MessageBubble::eventFilter(QObject *watched, QEvent *event) {
  if (watched == clickable_widget_ &&
      event->type() == QEvent::MouseButtonRelease) {
    auto *mouse_event = static_cast<QMouseEvent *>(event);
    if (mouse_event->button() == Qt::LeftButton && file_id_ > 0) {
      if (type_ == Image) {
        emit imageOpenRequested(file_id_, file_name_, file_size_);
      } else if (type_ == File) {
        emit fileDownloadRequested(file_id_, file_name_, file_size_);
      }
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

QWidget *MessageBubble::CreateContentWidget(const QString &content) {
  switch (type_) {
    case Image:
      return CreateImageContent(content);
    case File:
      return CreateFileContent(content);
    case Text:
    default:
      return CreateTextContent(content);
  }
}

QWidget *MessageBubble::CreateTextContent(const QString &content) {
  bubble_ = new QLabel(content, this);
  auto *label = qobject_cast<QLabel *>(bubble_);
  label->setObjectName(is_self_ ? "sentBubble" : "receivedBubble");
  label->setWordWrap(true);
  label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return label;
}

QWidget *MessageBubble::CreateImageContent(const QString &content) {
  const QJsonObject object = ParseContentJson(content);
  file_id_ = object.value("fileId").toInt();
  file_name_ = object.value("fileName").toString(QStringLiteral("image"));
  file_size_ = static_cast<qint64>(object.value("fileSize").toDouble());

  thumbnail_label_ = new QLabel(this);
  bubble_ = thumbnail_label_;
  thumbnail_label_->setObjectName("imageBubble");
  thumbnail_label_->setAlignment(Qt::AlignCenter);
  thumbnail_label_->setFixedSize(kPreviewSize, kPreviewSize);
  thumbnail_label_->setScaledContents(false);
  thumbnail_label_->setText(QStringLiteral("\u56fe\u7247\u52a0\u8f7d\u4e2d"));
  thumbnail_label_->setToolTip(QStringLiteral("\u70b9\u51fb\u67e5\u770b\u539f\u56fe"));
  thumbnail_label_->installEventFilter(this);
  clickable_widget_ = thumbnail_label_;

  LoadThumbnail();
  return thumbnail_label_;
}

QWidget *MessageBubble::CreateFileContent(const QString &content) {
  const QJsonObject object = ParseContentJson(content);
  file_id_ = object.value("fileId").toInt();
  file_name_ = object.value("fileName").toString(QStringLiteral("file"));
  file_size_ = static_cast<qint64>(object.value("fileSize").toDouble());

  auto *frame = new QFrame(this);
  auto *layout = new QHBoxLayout(frame);
  auto *icon = new QLabel(FileIconText(file_name_), frame);
  auto *text_layout = new QVBoxLayout();
  auto *name_label = new QLabel(file_name_, frame);
  auto *size_label =
      new QLabel(QStringLiteral("\u5927\u5c0f\uff1a%1")
                     .arg(ReadableFileSize(file_size_)),
                 frame);
  auto *download_label =
      new QLabel(QStringLiteral("\u70b9\u51fb\u4e0b\u8f7d"), frame);

  bubble_ = frame;
  frame->setObjectName("fileBubble");
  frame->setCursor(Qt::PointingHandCursor);
  frame->setToolTip(QStringLiteral("\u70b9\u51fb\u4e0b\u8f7d"));
  frame->installEventFilter(this);
  clickable_widget_ = frame;

  icon->setObjectName("fileTypeIcon");
  icon->setAlignment(Qt::AlignCenter);
  icon->setFixedSize(42, 42);
  icon->setStyleSheet(QStringLiteral(
                          "QLabel#fileTypeIcon { background: %1; color: "
                          "#ffffff; border-radius: 6px; font-weight: 700; }")
                          .arg(FileIconColor(file_name_)));
  name_label->setObjectName("fileNameLabel");
  size_label->setObjectName("fileSizeLabel");
  download_label->setObjectName("fileDownloadLabel");

  text_layout->setContentsMargins(0, 0, 0, 0);
  text_layout->setSpacing(3);
  text_layout->addWidget(name_label);
  text_layout->addWidget(size_label);
  text_layout->addWidget(download_label);

  layout->setContentsMargins(12, 10, 12, 10);
  layout->setSpacing(10);
  layout->addWidget(icon);
  layout->addLayout(text_layout);

  return frame;
}

void MessageBubble::LoadThumbnail() {
  if (file_id_ <= 0 || thumbnail_label_ == nullptr) {
    return;
  }

  ApiClient::instance()->downloadBytes(
      QStringLiteral("/api/v1/files/thumbnail/%1").arg(file_id_), this,
      [this](const QByteArray &data) {
        QPixmap pixmap;
        if (!pixmap.loadFromData(data)) {
          thumbnail_label_->setText(QStringLiteral("\u56fe\u7247\u52a0\u8f7d\u5931\u8d25"));
          return;
        }
        thumbnail_label_->setPixmap(
            RoundedPixmap(pixmap, QSize(kPreviewSize, kPreviewSize), 8));
      },
      [this]() {
        if (thumbnail_label_ != nullptr) {
          thumbnail_label_->setText(QStringLiteral("\u56fe\u7247\u52a0\u8f7d\u5931\u8d25"));
        }
      });
}
