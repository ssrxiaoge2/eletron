#include "FriendTabWidget.h"

#include "../src/ApiClient.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QHash>
#include <functional>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

namespace {

QString DisplayName(const QJsonObject &item) {
  const QString nickname = item.value("nickname").toString();
  return nickname.isEmpty() ? item.value("username").toString() : nickname;
}

QString DotStyle(bool online) {
  return QStringLiteral("border-radius: 4px; background-color: %1;")
      .arg(online ? QStringLiteral("#26c35a") : QStringLiteral("#9a9a9a"));
}

class HeaderWidget : public QWidget {
 public:
  explicit HeaderWidget(QWidget *parent = nullptr) : QWidget(parent) {}
  std::function<void()> on_click;

 protected:
  void mouseReleaseEvent(QMouseEvent *event) override {
    QWidget::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton && on_click) {
      on_click();
    }
  }
};

}  // namespace

FriendTabWidget::FriendTabWidget(QWidget *parent) : QWidget(parent) {
  auto *root_layout = new QVBoxLayout(this);
  auto *scroll_area = new QScrollArea(this);
  auto *container = new QWidget(scroll_area);
  auto *add_group_button =
      new QPushButton(QStringLiteral("+ \u65b0\u5efa\u5206\u7ec4"), this);

  content_layout_ = new QVBoxLayout(container);
  add_group_button->setObjectName("btnAddGroup");
  scroll_area->setWidgetResizable(true);
  scroll_area->setFrameShape(QFrame::NoFrame);
  scroll_area->setWidget(container);
  container->setObjectName("friendManagerContent");
  content_layout_->setContentsMargins(0, 0, 0, 0);
  content_layout_->setSpacing(0);
  content_layout_->addStretch();

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);
  root_layout->addWidget(scroll_area, 1);
  root_layout->addWidget(add_group_button);

  connect(add_group_button, &QPushButton::clicked, this,
          &FriendTabWidget::AddGroup);
}

void FriendTabWidget::loadGroups() {
  ApiClient::instance()->get(
      "/api/v1/friend-groups", this,
      [this](const QJsonObject &response) {
        RenderGroups(response.value("data").toArray());
      });
}

void FriendTabWidget::RenderGroups(const QJsonArray &groups) {
  while (content_layout_->count() > 1) {
    QLayoutItem *item = content_layout_->takeAt(0);
    if (item->widget() != nullptr) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  for (const QJsonValue &value : groups) {
    content_layout_->insertWidget(content_layout_->count() - 1,
                                  CreateGroupBlock(value.toObject(), groups));
  }
}

QWidget *FriendTabWidget::CreateGroupBlock(const QJsonObject &group,
                                           const QJsonArray &groups) {
  auto *block = new QWidget(this);
  auto *layout = new QVBoxLayout(block);
  auto *header = new HeaderWidget(block);
  auto *header_layout = new QHBoxLayout(header);
  auto *arrow_label = new QLabel(QStringLiteral("\u25bc"), header);
  auto *title_label = new QLabel(group.value("name").toString(), header);
  auto *count_label =
      new QLabel(QStringLiteral("%1/%2")
                     .arg(group.value("onlineCount").toInt())
                     .arg(group.value("totalCount").toInt()),
                 header);
  auto *menu_button = new QToolButton(header);
  auto *friends_container = new QWidget(block);
  auto *friends_layout = new QVBoxLayout(friends_container);

  header->setObjectName("groupHeader");
  title_label->setObjectName("groupTitle");
  count_label->setObjectName("groupCount");
  menu_button->setText("...");

  header_layout->setContentsMargins(12, 8, 8, 8);
  header_layout->addWidget(arrow_label);
  header_layout->addWidget(title_label);
  header_layout->addWidget(count_label);
  header_layout->addStretch();
  header_layout->addWidget(menu_button);

  friends_layout->setContentsMargins(0, 0, 0, 0);
  friends_layout->setSpacing(0);
  const QJsonArray friends = group.value("friends").toArray();
  for (const QJsonValue &value : friends) {
    friends_layout->addWidget(CreateFriendItem(value.toObject(), groups));
  }
  if (friends.isEmpty()) {
    auto *empty_label = new QLabel(QStringLiteral("\u6682\u65e0\u597d\u53cb"),
                                   friends_container);
    empty_label->setObjectName("groupCount");
    empty_label->setContentsMargins(26, 10, 0, 10);
    friends_layout->addWidget(empty_label);
  }

  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(header);
  layout->addWidget(friends_container);

  connect(menu_button, &QToolButton::clicked, this,
          [this, group, menu_button]() {
            ShowGroupMenu(group.value("groupId").toInt(),
                          group.value("name").toString(), menu_button);
          });
  header->setCursor(Qt::PointingHandCursor);
  header->on_click = [arrow_label, friends_container]() {
    const bool visible = friends_container->isVisible();
    friends_container->setVisible(!visible);
    arrow_label->setText(visible ? QStringLiteral("\u25b6")
                                 : QStringLiteral("\u25bc"));
  };
  return block;
}

QWidget *FriendTabWidget::CreateFriendItem(const QJsonObject &friend_item,
                                           const QJsonArray &groups) {
  const QString name = DisplayName(friend_item);
  auto *item = new QWidget(this);
  auto *layout = new QHBoxLayout(item);
  auto *avatar_box = new QWidget(item);
  auto *avatar = new QLabel(name.left(1).toUpper(), avatar_box);
  auto *online_dot = new QLabel(avatar_box);
  auto *text_layout = new QVBoxLayout();
  auto *name_label = new QLabel(name, item);
  auto *signature_label =
      new QLabel(friend_item.value("signature").toString(), item);

  item->setObjectName("friendItem");
  item->setFixedHeight(56);
  item->setContextMenuPolicy(Qt::CustomContextMenu);
  avatar->setObjectName("friendAvatar");
  avatar->setFixedSize(40, 40);
  avatar->setAlignment(Qt::AlignCenter);
  online_dot->setFixedSize(8, 8);
  online_dot->setStyleSheet(DotStyle(friend_item.value("isOnline").toBool()));
  online_dot->setAttribute(Qt::WA_StyledBackground, true);
  avatar_box->setFixedSize(44, 44);
  avatar->move(0, 0);
  online_dot->move(32, 32);
  name_label->setObjectName("friendName");
  signature_label->setObjectName("friendSignature");

  text_layout->setContentsMargins(0, 0, 0, 0);
  text_layout->setSpacing(3);
  text_layout->addWidget(name_label);
  text_layout->addWidget(signature_label);
  layout->setContentsMargins(14, 6, 10, 6);
  layout->setSpacing(10);
  layout->addWidget(avatar_box);
  layout->addLayout(text_layout, 1);

  connect(item, &QWidget::customContextMenuRequested, this,
          [this, friend_item, groups, item]() {
            ShowFriendMenu(friend_item, groups, item);
          });
  return item;
}

void FriendTabWidget::ShowGroupMenu(int group_id, const QString &group_name,
                                    QWidget *anchor) {
  QMenu menu(this);
  QAction *rename_action =
      menu.addAction(QStringLiteral("\u91cd\u547d\u540d\u5206\u7ec4"));
  QAction *delete_action =
      menu.addAction(QStringLiteral("\u5220\u9664\u5206\u7ec4"));
  QAction *selected =
      menu.exec(anchor->mapToGlobal(QPoint(0, anchor->height())));
  if (selected == rename_action) {
    RenameGroup(group_id, group_name);
  } else if (selected == delete_action) {
    DeleteGroup(group_id);
  }
}

void FriendTabWidget::ShowFriendMenu(const QJsonObject &friend_item,
                                     const QJsonArray &groups,
                                     QWidget *anchor) {
  QMenu menu(this);
  QAction *message_action =
      menu.addAction(QStringLiteral("\u53d1\u9001\u6d88\u606f"));
  QMenu *move_menu = menu.addMenu(QStringLiteral("\u79fb\u52a8\u5230\u5206\u7ec4"));
  QAction *delete_action =
      menu.addAction(QStringLiteral("\u5220\u9664\u597d\u53cb"));
  QHash<QAction *, int> group_actions;
  for (const QJsonValue &value : groups) {
    const QJsonObject group = value.toObject();
    QAction *action = move_menu->addAction(group.value("name").toString());
    group_actions.insert(action, group.value("groupId").toInt());
  }

  QAction *selected =
      menu.exec(anchor->mapToGlobal(QPoint(0, anchor->height())));
  if (selected == message_action) {
    emit friendMessageRequested(friend_item.value("userId").toInt(),
                                DisplayName(friend_item),
                                friend_item.value("isOnline").toBool());
  } else if (group_actions.contains(selected)) {
    MoveFriendToGroup(friend_item.value("userId").toInt(),
                      group_actions.value(selected));
  } else if (selected == delete_action) {
    QMessageBox::information(this, QString(),
                             QStringLiteral("\u540e\u7aef\u672a\u63d0\u4f9b\u5220\u9664\u597d\u53cb\u63a5\u53e3"));
  }
}

void FriendTabWidget::AddGroup() {
  bool ok = false;
  const QString name = QInputDialog::getText(
      this, QStringLiteral("\u65b0\u5efa\u5206\u7ec4"),
      QStringLiteral("\u8bf7\u8f93\u5165\u5206\u7ec4\u540d\u79f0\uff1a"),
      QLineEdit::Normal, QString(), &ok);
  if (!ok || name.trimmed().isEmpty()) {
    return;
  }
  QJsonObject body;
  body.insert("name", name.trimmed());
  ApiClient::instance()->post("/api/v1/friend-groups", body, this,
                              [this](const QJsonObject &) { loadGroups(); });
}

void FriendTabWidget::RenameGroup(int group_id, const QString &old_name) {
  bool ok = false;
  const QString name = QInputDialog::getText(
      this, QStringLiteral("\u91cd\u547d\u540d\u5206\u7ec4"),
      QStringLiteral("\u5206\u7ec4\u540d\u79f0\uff1a"), QLineEdit::Normal,
      old_name, &ok);
  if (!ok || name.trimmed().isEmpty()) {
    return;
  }
  QJsonObject body;
  body.insert("name", name.trimmed());
  ApiClient::instance()->put(
      QStringLiteral("/api/v1/friend-groups/%1").arg(group_id), body, this,
      [this](const QJsonObject &) { loadGroups(); });
}

void FriendTabWidget::DeleteGroup(int group_id) {
  const QMessageBox::StandardButton choice = QMessageBox::question(
      this, QStringLiteral("\u5220\u9664\u5206\u7ec4"),
      QStringLiteral("\u786e\u5b9a\u5220\u9664\u8be5\u5206\u7ec4\uff1f"));
  if (choice != QMessageBox::Yes) {
    return;
  }
  ApiClient::instance()->deleteResource(
      QStringLiteral("/api/v1/friend-groups/%1").arg(group_id), this,
      [this](const QJsonObject &) { loadGroups(); });
}

void FriendTabWidget::MoveFriendToGroup(int friend_user_id, int group_id) {
  QJsonObject body;
  body.insert("groupId", group_id);
  ApiClient::instance()->put(
      QStringLiteral("/api/v1/friends/%1/group").arg(friend_user_id), body,
      this, [this](const QJsonObject &) { loadGroups(); });
}
