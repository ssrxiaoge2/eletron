#include "ImageViewerDialog.h"

#include <QtCore/QFileInfo>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QScreen>
#include <QtGui/QWheelEvent>
#include <QtCore/QSizeF>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace {

QString ReadableFileSize(qint64 size) {
  if (size < 1024) {
    return QStringLiteral("%1 B").arg(size);
  }
  if (size < 1024 * 1024) {
    return QStringLiteral("%1 KB").arg(size / 1024.0, 0, 'f', 1);
  }
  return QStringLiteral("%1 MB").arg(size / 1024.0 / 1024.0, 0, 'f', 1);
}

class ImageCanvas : public QWidget {
 public:
  explicit ImageCanvas(const QPixmap &pixmap, QWidget *parent = nullptr)
      : QWidget(parent), pixmap_(pixmap) {
    setMouseTracking(true);
    setCursor(Qt::OpenHandCursor);
  }

 protected:
  void paintEvent(QPaintEvent *event) override {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#111111"));
    if (pixmap_.isNull()) {
      return;
    }

    const QSizeF available(width() * 0.9, height() * 0.9);
    const double base_scale =
        qMin(1.0, qMin(available.width() / pixmap_.width(),
                       available.height() / pixmap_.height()));
    const QSize scaled_size =
        (QSizeF(pixmap_.size()) * base_scale * scale_).toSize();
    const QPixmap scaled =
        pixmap_.scaled(scaled_size, Qt::KeepAspectRatio,
                       Qt::SmoothTransformation);
    const QPoint top_left((width() - scaled.width()) / 2 + offset_.x(),
                          (height() - scaled.height()) / 2 + offset_.y());
    painter.drawPixmap(top_left, scaled);
  }

  void wheelEvent(QWheelEvent *event) override {
    const double delta = event->angleDelta().y() > 0 ? 0.1 : -0.1;
    scale_ = qBound(0.2, scale_ + delta, 5.0);
    update();
  }

  void mousePressEvent(QMouseEvent *event) override {
    if (event->button() == Qt::LeftButton) {
      dragging_ = true;
      drag_start_ = event->pos();
      setCursor(Qt::ClosedHandCursor);
    }
  }

  void mouseMoveEvent(QMouseEvent *event) override {
    if (dragging_) {
      offset_ += event->pos() - drag_start_;
      drag_start_ = event->pos();
      update();
    }
  }

  void mouseReleaseEvent(QMouseEvent *event) override {
    if (event->button() == Qt::LeftButton) {
      dragging_ = false;
      setCursor(Qt::OpenHandCursor);
    }
  }

 private:
  QPixmap pixmap_;
  double scale_ = 1.0;
  QPoint offset_;
  QPoint drag_start_;
  bool dragging_ = false;
};

}  // namespace

ImageViewerDialog::ImageViewerDialog(const QPixmap &pixmap,
                                     const QString &file_name, qint64 file_size,
                                     QWidget *parent)
    : QDialog(parent), pixmap_(pixmap), file_name_(file_name) {
  setObjectName("imageViewerDialog");
  setWindowTitle(file_name);
  setModal(false);
  setWindowState(Qt::WindowFullScreen);

  QRect screen_geometry = QApplication::primaryScreen()->availableGeometry();
  resize(screen_geometry.width() * 0.9, screen_geometry.height() * 0.9);

  auto *root_layout = new QVBoxLayout(this);
  auto *top_bar = new QHBoxLayout();
  auto *close_button =
      new QPushButton(QStringLiteral("\u5173\u95ed"), this);
  auto *canvas = new ImageCanvas(pixmap, this);
  auto *bottom_bar = new QHBoxLayout();
  auto *info_label = new QLabel(
      QStringLiteral("%1  %2x%3  %4")
          .arg(file_name)
          .arg(pixmap.width())
          .arg(pixmap.height())
          .arg(ReadableFileSize(file_size)),
      this);
  auto *save_button =
      new QPushButton(QStringLiteral("\u4fdd\u5b58\u5230\u672c\u5730"), this);
  auto *bottom_close_button =
      new QPushButton(QStringLiteral("\u5173\u95ed"), this);

  root_layout->setContentsMargins(16, 12, 16, 12);
  root_layout->setSpacing(10);
  top_bar->addStretch();
  top_bar->addWidget(close_button);
  bottom_bar->addWidget(info_label, 1);
  bottom_bar->addWidget(save_button);
  bottom_bar->addWidget(bottom_close_button);

  root_layout->addLayout(top_bar);
  root_layout->addWidget(canvas, 1);
  root_layout->addLayout(bottom_bar);

  connect(close_button, &QPushButton::clicked, this, &QDialog::close);
  connect(bottom_close_button, &QPushButton::clicked, this, &QDialog::close);
  connect(save_button, &QPushButton::clicked, this, [this]() {
    const QString save_path = QFileDialog::getSaveFileName(
        this, QStringLiteral("\u4fdd\u5b58\u56fe\u7247"), file_name_);
    if (!save_path.isEmpty()) {
      pixmap_.save(save_path);
    }
  });
}
