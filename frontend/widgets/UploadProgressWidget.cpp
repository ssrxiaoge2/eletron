#include "UploadProgressWidget.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

UploadProgressWidget::UploadProgressWidget(QWidget *parent) : QWidget(parent) {
  setObjectName("uploadProgressWidget");
  setVisible(false);

  auto *root_layout = new QVBoxLayout(this);
  auto *progress_row = new QHBoxLayout();
  auto *cancel_button =
      new QPushButton(QStringLiteral("\u53d6\u6d88"), this);

  title_label_ = new QLabel(this);
  progress_bar_ = new QProgressBar(this);
  progress_bar_->setRange(0, 100);
  progress_bar_->setTextVisible(true);

  cancel_button->setObjectName("uploadCancelButton");
  root_layout->setContentsMargins(14, 8, 14, 8);
  root_layout->setSpacing(6);
  progress_row->setContentsMargins(0, 0, 0, 0);
  progress_row->setSpacing(8);
  progress_row->addWidget(progress_bar_, 1);
  progress_row->addWidget(cancel_button);

  root_layout->addWidget(title_label_);
  root_layout->addLayout(progress_row);

  connect(cancel_button, &QPushButton::clicked, this,
          &UploadProgressWidget::cancelRequested);
}

void UploadProgressWidget::Start(const QString &file_name) {
  title_label_->setText(
      QStringLiteral("\u6b63\u5728\u4e0a\u4f20\uff1a%1").arg(file_name));
  progress_bar_->setValue(0);
  setVisible(true);
}

void UploadProgressWidget::SetProgress(qint64 sent, qint64 total) {
  if (total <= 0) {
    progress_bar_->setValue(0);
    return;
  }
  progress_bar_->setValue(static_cast<int>(sent * 100 / total));
}

void UploadProgressWidget::Finish() {
  progress_bar_->setValue(100);
  setVisible(false);
}
