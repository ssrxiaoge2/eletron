#include "GroupMemberWidget.h"

#include "MuteDialog.h"
#include "../src/ApiClient.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QVBoxLayout>

namespace {

constexpr int kMemberDataRole = Qt::UserRole + 1;

QString DisplayName(const QJsonObject &member) {
  const QString nickname = member.value("nickname").toString();
  return nickname.isEmpty() ? member.value("username").toString() : nickname;
}

QString RoleText(int role) {
  if (role == 2) {
    return QStringLiteral("\U0001f451 ");
  }
  if (role == 1) {
    return QStringLiteral("\U0001f527 ");
  }
  return QString();
}

class MemberItemWidget : public QWidget {
 public:
  explicit MemberItemWidget(const QJsonObject &member,
                            QWidget *parent = nullptr)
      : QWidget(parent) {
    const QString name = DisplayName(member);
    auto *root_layout = new QHBoxLayout(this);
    auto *avatar = new QLabel(name.left(1).toUpper(), this);
    auto *name_label = new QLabel(RoleText(member.value("role").toInt()) + name,
                                  this);
    auto *mute_label = new QLabel(QStringLiteral("\U0001f507"), this);

    avatar->setObjectName("friendAvatar");
    avatar->setFixedSize(32, 32);
    avatar->setAlignment(Qt::AlignCenter);
    name_label->setObjectName("friendName");
    mute_label->setVisible(member.value("isMuted").toBool(false));

    root_layout->setContentsMargins(8, 6, 8, 6);
    root_layout->setSpacing(8);
    root_layout->addWidget(avatar);
    root_layout->addWidget(name_label, 1);
    root_layout->addWidget(mute_label);
  }
};

}  // namespace

GroupMemberWidget::GroupMemberWidget(QWidget *parent) : QWidget(parent) {
  auto *root_layout = new QVBoxLayout(this);
  auto *title = new QLabel(QStringLiteral("\u7fa4\u804a\u6210\u5458"), this);
  search_edit_ = new QLineEdit(this);
  member_list_ = new QListWidget(this);

  setObjectName("groupMemberPanel");
  title->setObjectName("sidePanelTitle");
  search_edit_->setPlaceholderText(QStringLiteral("\u641c\u7d22"));
  member_list_->setFrameShape(QFrame::NoFrame);
  member_list_->setContextMenuPolicy(Qt::CustomContextMenu);

  root_layout->setContentsMargins(12, 12, 12, 12);
  root_layout->setSpacing(8);
  root_layout->addWidget(title);
  root_layout->addWidget(search_edit_);
  root_layout->addWidget(member_list_, 1);

  connect(member_list_, &QListWidget::customContextMenuRequested, this,
          &GroupMemberWidget::ShowContextMenu);
}

void GroupMemberWidget::setGroup(int group_id, int my_role,
                                 int current_user_id) {
  group_id_ = group_id;
  my_role_ = my_role;
  current_user_id_ = current_user_id;
  loadMembers();
}

void GroupMemberWidget::loadMembers() {
  member_list_->clear();
  if (group_id_ <= 0) {
    return;
  }
  ApiClient::instance()->get(
      QStringLiteral("/api/v1/groups/%1/members").arg(group_id_), this,
      [this](const QJsonObject &response) {
        const QJsonArray members = response.value("data").toArray();
        for (const QJsonValue &value : members) {
          AddMember(value.toObject());
        }
        emit membersChanged(members.size());
      });
}

void GroupMemberWidget::AddMember(const QJsonObject &member) {
  auto *item = new QListWidgetItem();
  item->setData(kMemberDataRole, member);
  item->setSizeHint({190, 48});
  member_list_->addItem(item);
  member_list_->setItemWidget(item, new MemberItemWidget(member, member_list_));
}

void GroupMemberWidget::ShowContextMenu(const QPoint &pos) {
  QListWidgetItem *item = member_list_->itemAt(pos);
  if (item == nullptr) {
    return;
  }
  const QJsonObject member = item->data(kMemberDataRole).toJsonObject();
  const int user_id = member.value("userId").toInt();
  const int role = member.value("role").toInt();

  QMenu menu(this);
  QAction *profile_action =
      menu.addAction(QStringLiteral("\u67e5\u770b\u8d44\u6599"));
  QAction *role_action = nullptr;
  QAction *mute_action = nullptr;
  QAction *kick_action = nullptr;
  const bool can_manage = user_id != current_user_id_ &&
                          (my_role_ == 2 || (my_role_ == 1 && role == 0));
  if (my_role_ == 2 && user_id != current_user_id_) {
    role_action = menu.addAction(
        role == 1 ? QStringLiteral("\u53d6\u6d88\u7ba1\u7406\u5458")
                  : QStringLiteral("\u8bbe\u4e3a\u7ba1\u7406\u5458"));
  }
  if (can_manage) {
    mute_action = menu.addAction(QStringLiteral("\u7981\u8a00"));
    kick_action = menu.addAction(QStringLiteral("\u8e22\u51fa\u7fa4\u804a"));
  }

  QAction *selected = menu.exec(member_list_->viewport()->mapToGlobal(pos));
  if (selected == profile_action) {
    QMessageBox::information(this, QStringLiteral("\u6210\u5458\u8d44\u6599"),
                             DisplayName(member));
  } else if (role_action != nullptr && selected == role_action) {
    SetRole(user_id, role == 1 ? 0 : 1);
  } else if (mute_action != nullptr && selected == mute_action) {
    MuteMember(member);
  } else if (kick_action != nullptr && selected == kick_action) {
    KickMember(member);
  }
}

void GroupMemberWidget::SetRole(int user_id, int role) {
  QJsonObject body;
  body.insert("role", role);
  ApiClient::instance()->put(
      QStringLiteral("/api/v1/groups/%1/members/%2/role").arg(group_id_).arg(user_id),
      body, this, [this](const QJsonObject &) { loadMembers(); },
      [this]() {
        QMessageBox::warning(this, QString(),
                             QStringLiteral("\u8bbe\u7f6e\u7ba1\u7406\u5458\u5931\u8d25"));
      });
}

void GroupMemberWidget::MuteMember(const QJsonObject &member) {
  MuteDialog dialog(DisplayName(member), member.value("isMuted").toBool(false),
                    this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  QJsonObject body;
  body.insert("duration", dialog.durationSeconds());
  ApiClient::instance()->put(
      QStringLiteral("/api/v1/groups/%1/members/%2/mute")
          .arg(group_id_)
          .arg(member.value("userId").toInt()),
      body, this, [this](const QJsonObject &) { loadMembers(); },
      [this]() {
        QMessageBox::warning(this, QString(),
                             QStringLiteral("\u7981\u8a00\u5931\u8d25"));
      });
}

void GroupMemberWidget::KickMember(const QJsonObject &member) {
  const QMessageBox::StandardButton choice = QMessageBox::question(
      this, QStringLiteral("\u8e22\u51fa\u6210\u5458"),
      QStringLiteral("\u786e\u5b9a\u5c06\u300c%1\u300d\u8e22\u51fa\u7fa4\u804a\uff1f")
          .arg(DisplayName(member)));
  if (choice != QMessageBox::Yes) {
    return;
  }
  ApiClient::instance()->deleteResource(
      QStringLiteral("/api/v1/groups/%1/members/%2")
          .arg(group_id_)
          .arg(member.value("userId").toInt()),
      this, [this](const QJsonObject &) { loadMembers(); },
      [this]() {
        QMessageBox::warning(this, QString(),
                             QStringLiteral("\u8e22\u51fa\u6210\u5458\u5931\u8d25"));
      });
}
