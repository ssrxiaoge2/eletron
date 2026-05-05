#include "CreateGroupDialog.h"

#include "../src/ApiClient.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace {

constexpr int kUserIdRole = Qt::UserRole + 1;

QString DisplayName(const QJsonObject &item) {
  const QString nickname = item.value("nickname").toString();
  return nickname.isEmpty() ? item.value("username").toString() : nickname;
}

}  // namespace

CreateGroupDialog::CreateGroupDialog(int default_user_id,
                                     const QString &default_display_name,
                                     QWidget *parent)
    : QDialog(parent), default_user_id_(default_user_id) {
  setWindowTitle(QStringLiteral("\u521b\u5efa\u7fa4\u804a"));
  setModal(true);
  setFixedWidth(480);
  setStyleSheet(R"(
    QDialog {
      background-color: #ffffff;
    }
    QLabel {
      color: #222222;
    }
    QLineEdit {
      background-color: #f5f5f5;
      border: 1px solid #dddddd;
      border-radius: 4px;
      padding: 6px 10px;
      color: #222222;
      font-size: 13px;
    }
    QListWidget {
      background-color: #f8f8f8;
      border: 1px solid #dddddd;
      border-radius: 4px;
      color: #222222;
    }
    QListWidget::item {
      height: 36px;
      padding-left: 8px;
      color: #222222;
    }
    QListWidget::item:hover {
      background-color: #e8f0fb;
    }
    QCheckBox {
      color: #222222;
      spacing: 8px;
    }
    QLabel#countLabel {
      color: #888888;
      font-size: 12px;
    }
    QPushButton#btnCreate {
      background-color: #4a90d9;
      color: #ffffff;
      border-radius: 4px;
      padding: 6px 20px;
    }
    QPushButton#btnCreate:disabled {
      background-color: #b0c4de;
    }
    QPushButton#btnCancel {
      background-color: #f0f0f0;
      color: #444444;
      border-radius: 4px;
      padding: 6px 20px;
    }
  )");

  auto *root_layout = new QVBoxLayout(this);
  auto *title = new QLabel(QStringLiteral("\u521b\u5efa\u7fa4\u804a"), this);
  auto *name_row = new QWidget(this);
  auto *name_layout = new QHBoxLayout(name_row);
  auto *name_label = new QLabel(QStringLiteral("\u7fa4\u804a\u540d\u79f0\uff1a"),
                                name_row);
  auto *member_title =
      new QLabel(QStringLiteral("\u5df2\u9009\u6210\u5458"), this);
  auto *button_row = new QWidget(this);
  auto *button_layout = new QHBoxLayout(button_row);
  auto *cancel_button =
      new QPushButton(QStringLiteral("\u53d6\u6d88"), button_row);

  name_edit_ = new QLineEdit(name_row);
  member_list_ = new QListWidget(this);
  hint_label_ = new QLabel(this);
  create_button_ =
      new QPushButton(QStringLiteral("\u521b\u5efa\u7fa4\u804a"), button_row);

  title->setObjectName("dialogTitle");
  hint_label_->setObjectName("countLabel");
  create_button_->setObjectName("btnCreate");
  cancel_button->setObjectName("btnCancel");
  member_list_->setMinimumHeight(260);
  hint_label_->setObjectName("secondaryText");

  name_layout->setContentsMargins(0, 0, 0, 0);
  name_layout->addWidget(name_label);
  name_layout->addWidget(name_edit_, 1);

  button_layout->setContentsMargins(0, 0, 0, 0);
  button_layout->addStretch();
  button_layout->addWidget(cancel_button);
  button_layout->addWidget(create_button_);

  root_layout->setContentsMargins(24, 20, 24, 20);
  root_layout->setSpacing(14);
  root_layout->addWidget(title);
  root_layout->addWidget(name_row);
  root_layout->addWidget(member_title);
  root_layout->addWidget(member_list_);
  root_layout->addWidget(hint_label_);
  root_layout->addWidget(button_row);

  connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);
  connect(create_button_, &QPushButton::clicked, this,
          &CreateGroupDialog::CreateGroup);
  connect(name_edit_, &QLineEdit::textChanged, this,
          &CreateGroupDialog::UpdateCreateState);
  connect(member_list_, &QListWidget::itemChanged, this,
          &CreateGroupDialog::UpdateCreateState);

  if (default_user_id_ > 0) {
    AddFriend(default_user_id_, default_display_name);
  }
  LoadFriends();
  UpdateCreateState();
}

void CreateGroupDialog::LoadFriends() {
  ApiClient::instance()->get(
      "/api/v1/friends", this,
      [this](const QJsonObject &response) {
        const QJsonArray friends = response.value("data").toArray();
        for (const QJsonValue &value : friends) {
          const QJsonObject item = value.toObject();
          AddFriend(item.value("userId").toInt(), DisplayName(item));
        }
        UpdateCreateState();
      },
      [this]() {
        QMessageBox::warning(this, QString(),
                             QStringLiteral("\u52a0\u8f7d\u597d\u53cb\u5931\u8d25"));
      });
}

void CreateGroupDialog::AddFriend(int user_id, const QString &display_name) {
  if (user_id <= 0) {
    return;
  }
  for (int i = 0; i < member_list_->count(); ++i) {
    if (member_list_->item(i)->data(kUserIdRole).toInt() == user_id) {
      return;
    }
  }

  auto *item = new QListWidgetItem(display_name);
  item->setData(kUserIdRole, user_id);
  item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
  item->setCheckState(user_id == default_user_id_ ? Qt::Checked
                                                  : Qt::Unchecked);
  if (user_id == default_user_id_) {
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
  }
  member_list_->addItem(item);
}

void CreateGroupDialog::UpdateCreateState() {
  int selected_count = 0;
  for (int i = 0; i < member_list_->count(); ++i) {
    if (member_list_->item(i)->checkState() == Qt::Checked) {
      ++selected_count;
    }
  }

  const bool can_create = !name_edit_->text().trimmed().isEmpty() &&
                          selected_count >= 2;
  create_button_->setEnabled(can_create);
  hint_label_->setText(QStringLiteral("\u5df2\u9009 %1 \u4eba\uff08\u81f3\u5c11\u518d\u9009 1 \u4eba\uff09")
                           .arg(selected_count));
}

void CreateGroupDialog::CreateGroup() {
  QJsonArray member_ids;
  for (int i = 0; i < member_list_->count(); ++i) {
    QListWidgetItem *item = member_list_->item(i);
    if (item->checkState() == Qt::Checked) {
      member_ids.append(item->data(kUserIdRole).toInt());
    }
  }

  QJsonObject body;
  body.insert("name", name_edit_->text().trimmed());
  body.insert("memberIds", member_ids);
  create_button_->setEnabled(false);
  ApiClient::instance()->post(
      "/api/v1/groups", body, this,
      [this](const QJsonObject &response) {
        const QJsonObject data = response.value("data").toObject();
        emit groupCreated(data.value("groupId").toInt(),
                          data.value("name").toString(name_edit_->text()),
                          data.value("memberCount").toInt());
        accept();
      },
      [this]() {
        create_button_->setEnabled(true);
        QMessageBox::warning(this, QString(),
                             QStringLiteral("\u521b\u5efa\u7fa4\u804a\u5931\u8d25"));
      });
}
