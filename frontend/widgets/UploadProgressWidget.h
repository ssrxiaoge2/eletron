#pragma once

#include <QtWidgets/QWidget>

class QLabel;
class QProgressBar;

class UploadProgressWidget : public QWidget {
  Q_OBJECT

 public:
  explicit UploadProgressWidget(QWidget *parent = nullptr);
  void Start(const QString &file_name);
  void SetProgress(qint64 sent, qint64 total);
  void Finish();

 signals:
  void cancelRequested();

 private:
  QLabel *title_label_;
  QProgressBar *progress_bar_;
};
