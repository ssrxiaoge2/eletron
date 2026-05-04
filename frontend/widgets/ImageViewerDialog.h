#pragma once

#include <QtGui/QPixmap>
#include <QtWidgets/QDialog>

class QLabel;

class ImageViewerDialog : public QDialog {
  Q_OBJECT

 public:
  ImageViewerDialog(const QPixmap &pixmap, const QString &file_name,
                    qint64 file_size, QWidget *parent = nullptr);

 private:
  QPixmap pixmap_;
  QString file_name_;
};
