#include "AddFriendDialog.h"

#include "../src/ApiClient.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QSet>
#include <QtCore/QUrl>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace {

QString DisplayName(const QString &username, const QString &nickname) {
  return nickname.isEmpty() ? username : nickname;
}

QString TextFailed() {
  return QStringLiteral("\u64cd\u4f5c\u5931\u8d25");
}

}  // namespace

AddFriendDialog::AddFriendDialog(QWidget *parent) : QDialog(parent) {
  setObjectName("darkDialog");
  setWindowTitle(QStringLiteral("\u6dfb\u52a0\u597d\u53cb"));
  setModal(true);
  setFixedWidth(400);

  auto *root_layout = new QVBoxLayout(this);
  auto *title = new QLabel(QStringLiteral("\u6dfb\u52a0\u597d\u53cb"), this);
  auto *search_row = new QWidget(this);
  auto *search_layout = new QHBoxLayout(search_row);
  auto *search_button = new QPushButton(QStringLiteral("\u641c\u7d22"), this);
  auto *close_button = new QPushButton(QStringLiteral("\u5173\u95ed"), this);

  search_edit_ = new QLineEdit(this);
  result_list_ = new QListWidget(this);

  title->setObjectName("dialogTitle");
  search_edit_->setPlaceholderText(QStringLiteral(
      "\u641c\u7d22\u7528\u6237\u540d / \u6635\u79f0"));
  result_list_->setObjectName("searchResultList");
  result_list_->setFrameShape(QFrame::NoFrame);

  search_layout->setContentsMargins(0, 0, 0, 0);
  search_layout->setSpacing(8);
  search_layout->addWidget(search_edit_, 1);
  search_layout->addWidget(search_button);

  root_layout->setContentsMargins(18, 18, 18, 18);
  root_layout->setSpacing(12);
  root_layout->addWidget(title);
  root_layout->addWidget(search_row);
  root_layout->addWidget(result_list_, 1);
  root_layout->addWidget(close_button, 0, Qt::AlignRight);

  connect(search_button, &QPushButton::clicked, this,
          &AddFriendDialog::SearchUsers);
  connect(search_edit_, &QLineEdit::returnPressed, this,
          &AddFriendDialog::SearchUsers);
  connect(close_button, &QPushButton::clicked, this, &QDialog::accept);
}

void AddFriendDialog::SearchUsers() {
  const QString keyword = search_edit_->text().trimmed();
  result_list_->clear();
  if (keyword.isEmpty()) {
    return;
  }
  const int generation = ++search_generation_;

  const QString path =
      QStringLiteral("/api/v1/users/search?keyword=%1")
          .arg(QString::fromUtf8(QUrl::toPercentEncoding(keyword)));
  ApiClient::instance()->get(path, this, [this, generation](const QJsonObject &response) {
    if (generation != search_generation_) {
      return;
    }
    result_list_->clear();
    const QJsonArray users = response.value("data").toArray();
    QSet<int> rendered_user_ids;
    for (const QJsonValue &value : users) {
      const QJsonObject user = value.toObject();
      const int user_id = user.value("userId").toInt();
      if (user_id <= 0 || rendered_user_ids.contains(user_id)) {
        continue;
      }
      rendered_user_ids.insert(user_id);
      AddResult(user_id,
                user.value("username").toString(),
                user.value("nickname").toString(),
                user.value("signature").toString(),
                user.value("friendStatus").toInt());
    }
  });
}

void AddFriendDialog::AddResult(int user_id, const QString &username,
                                const QString &nickname,
                                const QString &signature, int friend_status) {
  const QString name = DisplayName(username, nickname);
  auto *item = new QListWidgetItem();
  auto *row = new QWidget(result_list_);
  auto *row_layout = new QHBoxLayout(row);
  auto *avatar = new QLabel(name.left(1).toUpper(), row);
  auto *text_layout = new QVBoxLayout();
  auto *name_label = new QLabel(name, row);
  auto *username_label = new QLabel(username, row);
  auto *signature_label = new QLabel(signature, row);
  auto *button = new QPushButton(row);

  avatar->setObjectName("friendAvatar");
  name_label->setObjectName("friendName");
  username_label->setObjectName("friendSignature");
  signature_label->setObjectName("friendSignature");
  avatar->setFixedSize(36, 36);
  avatar->setAlignment(Qt::AlignCenter);

  if (friend_status == 0) {
    button->setText(QStringLiteral("\u6dfb\u52a0\u597d\u53cb"));
    connect(button, &QPushButton::clicked, this,
            [this, user_id, button]() { ApplyFriend(user_id, button); });
  } else if (friend_status == 3) {
    button->setText(QStringLiteral("\u63a5\u53d7\u7533\u8bf7"));
    button->setObjectName("acceptButton");
    connect(button, &QPushButton::clicked, this,
            [this, user_id, button]() { AcceptFriend(user_id, button); });
  } else {
    button->setEnabled(false);
    button->setObjectName("secondaryButton");
    button->setText(friend_status == 1
                        ? QStringLiteral("\u5df2\u662f\u597d\u53cb")
                        : QStringLiteral("\u5df2\u53d1\u9001\u7533\u8bf7"));
  }

  text_layout->setContentsMargins(0, 0, 0, 0);
  text_layout->setSpacing(2);
  text_layout->addWidget(name_label);
  text_layout->addWidget(username_label);
  text_layout->addWidget(signature_label);

  row_layout->setContentsMargins(10, 8, 10, 8);
  row_layout->setSpacing(10);
  row_layout->addWidget(avatar);
  row_layout->addLayout(text_layout, 1);
  row_layout->addWidget(button);

  item->setSizeHint({360, 82});
  result_list_->addItem(item);
  result_list_->setItemWidget(item, row);
}

void AddFriendDialog::ApplyFriend(int user_id, QPushButton *button) {
  QJsonObject body;
  body.insert("targetUserId", user_id);
  button->setEnabled(false);
  ApiClient::instance()->post(
      "/api/v1/friends/apply", body, this,
      [button](const QJsonObject &) {
        button->setObjectName("secondaryButton");
        button->setText(QStringLiteral("\u5df2\u53d1\u9001\u7533\u8bf7"));
        button->style()->unpolish(button);
        button->style()->polish(button);
      },
      [this, button]() {
        button->setEnabled(true);
        QMessageBox::warning(this, QString(), TextFailed());
      });
}

void AddFriendDialog::AcceptFriend(int user_id, QPushButton *button) {
  button->setEnabled(false);
  ApiClient::instance()->get(
      "/api/v1/friends/requests", this,
      [this, user_id, button](const QJsonObject &response) {
        const QJsonArray requests = response.value("data").toArray();
        int request_id = 0;
        for (const QJsonValue &value : requests) {
          const QJsonObject request = value.toObject();
          if (request.value("fromUserId").toInt() == user_id) {
            request_id = request.value("requestId").toInt();
            break;
          }
        }
        if (request_id <= 0) {
          button->setEnabled(true);
          QMessageBox::warning(this, QString(), TextFailed());
          return;
        }

        QJsonObject body;
        body.insert("requestId", request_id);
        body.insert("action", 1);
        ApiClient::instance()->post(
            "/api/v1/friends/handle", body, this,
            [this, button](const QJsonObject &) {
              button->setObjectName("secondaryButton");
              button->setText(QStringLiteral("\u5df2\u662f\u597d\u53cb"));
              button->style()->unpolish(button);
              button->style()->polish(button);
              emit friendsChanged();
            },
            [this, button]() {
              button->setEnabled(true);
              QMessageBox::warning(this, QString(), TextFailed());
            });
      },
      [this, button]() {
        button->setEnabled(true);
        QMessageBox::warning(this, QString(), TextFailed());
      });
}
