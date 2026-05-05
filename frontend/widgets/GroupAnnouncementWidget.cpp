#include "GroupAnnouncementWidget.h"

#include "../src/ApiClient.h"

#include <QtCore/QJsonObject>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

GroupAnnouncementWidget::GroupAnnouncementWidget(QWidget *parent)
    : QWidget(parent) {
  auto *root_layout = new QVBoxLayout(this);
  auto *header = new QWidget(this);
  auto *header_layout = new QHBoxLayout(header);
  auto *title = new QLabel(QStringLiteral("\u7fa4\u516c\u544a"), header);

  edit_button_ = new QToolButton(header);
  content_label_ = new QLabel(QStringLiteral("\u6682\u65e0\u7fa4\u516c\u544a"), this);

  setObjectName("AnnouncementWidget");
  setStyleSheet(R"(
    QWidget#AnnouncementWidget {
      background-color: #ffffff;
    }
    QLabel#announcementTitle {
      color: #222222;
      font-size: 14px;
      font-weight: bold;
    }
    QLabel#announcementContent {
      color: #444444;
      font-size: 13px;
    }
  )");
  title->setObjectName("announcementTitle");
  edit_button_->setText("+");
  content_label_->setObjectName("announcementContent");
  content_label_->setWordWrap(true);

  header_layout->setContentsMargins(0, 0, 0, 0);
  header_layout->addWidget(title);
  header_layout->addStretch();
  header_layout->addWidget(edit_button_);

  root_layout->setContentsMargins(12, 12, 12, 12);
  root_layout->setSpacing(8);
  root_layout->addWidget(header);
  root_layout->addWidget(content_label_);

  connect(edit_button_, &QToolButton::clicked, this,
          &GroupAnnouncementWidget::editAnnouncement);
}

void GroupAnnouncementWidget::setGroup(int group_id, bool can_edit) {
  group_id_ = group_id;
  can_edit_ = can_edit;
  edit_button_->setVisible(can_edit_);
  loadAnnouncement();
}

void GroupAnnouncementWidget::loadAnnouncement() {
  if (group_id_ <= 0) {
    content_label_->setText(QStringLiteral("\u6682\u65e0\u7fa4\u516c\u544a"));
    return;
  }
  ApiClient::instance()->get(
      QStringLiteral("/api/v1/groups/%1").arg(group_id_), this,
      [this](const QJsonObject &response) {
        const QString announcement =
            response.value("data").toObject().value("announcement").toString();
        content_label_->setText(announcement.isEmpty()
                                    ? QStringLiteral("\u6682\u65e0\u7fa4\u516c\u544a")
                                    : announcement);
      });
}

void GroupAnnouncementWidget::editAnnouncement() {
  if (!can_edit_ || group_id_ <= 0) {
    return;
  }

  QDialog dialog(this);
  dialog.setWindowTitle(QStringLiteral("\u53d1\u5e03\u7fa4\u516c\u544a"));
  dialog.setModal(true);
  dialog.setFixedWidth(420);
  auto *root_layout = new QVBoxLayout(&dialog);
  auto *edit = new QTextEdit(&dialog);
  auto *button_row = new QWidget(&dialog);
  auto *button_layout = new QHBoxLayout(button_row);
  auto *cancel_button = new QPushButton(QStringLiteral("\u53d6\u6d88"), button_row);
  auto *ok_button = new QPushButton(QStringLiteral("\u786e\u8ba4"), button_row);

  edit->setPlainText(content_label_->text() ==
                             QStringLiteral("\u6682\u65e0\u7fa4\u516c\u544a")
                         ? QString()
                         : content_label_->text());
  edit->setMinimumHeight(160);
  button_layout->setContentsMargins(0, 0, 0, 0);
  button_layout->addStretch();
  button_layout->addWidget(cancel_button);
  button_layout->addWidget(ok_button);
  root_layout->addWidget(edit);
  root_layout->addWidget(button_row);
  connect(cancel_button, &QPushButton::clicked, &dialog, &QDialog::reject);
  connect(ok_button, &QPushButton::clicked, &dialog, &QDialog::accept);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  QJsonObject body;
  body.insert("announcement", edit->toPlainText().trimmed());
  ApiClient::instance()->put(
      QStringLiteral("/api/v1/groups/%1/announcement").arg(group_id_), body,
      this,
      [this](const QJsonObject &) {
        loadAnnouncement();
        emit announcementChanged();
      },
      [this]() {
        QMessageBox::warning(this, QString(),
                             QStringLiteral("\u4fdd\u5b58\u7fa4\u516c\u544a\u5931\u8d25"));
      });
}
