#include "FriendManagerWidget.h"

#include "FriendTabWidget.h"
#include "GroupTabWidget.h"
#include "../src/ApiClient.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <functional>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

namespace {

class ClickableRow : public QWidget {
 public:
  explicit ClickableRow(QWidget *parent = nullptr) : QWidget(parent) {}
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

FriendManagerWidget::FriendManagerWidget(QWidget *parent) : QWidget(parent) {
  setObjectName("FriendManagerWidget");
  setStyleSheet(R"(
    QWidget {
      background-color: #ffffff;
      color: #222222;
    }
    QWidget#groupHeader {
      background-color: #f5f5f5;
      border-bottom: 1px solid #eeeeee;
    }
    QLabel#groupTitle {
      font-size: 13px;
      font-weight: bold;
      color: #444444;
    }
    QLabel#groupCount {
      font-size: 12px;
      color: #999999;
    }
    QWidget#friendItem:hover {
      background-color: #f0f5ff;
      border-radius: 4px;
    }
    QTabBar::tab {
      background: #f0f0f0;
      color: #666666;
      padding: 6px 24px;
      border: none;
    }
    QTabBar::tab:selected {
      background: #ffffff;
      color: #222222;
      border-bottom: 2px solid #4a90d9;
    }
    QPushButton#btnAddGroup {
      background: transparent;
      color: #4a90d9;
      border: none;
      text-align: left;
      padding: 8px 16px;
      font-size: 13px;
    }
    QPushButton#btnAddGroup:hover {
      background: #f0f5ff;
    }
  )");

  auto *root_layout = new QVBoxLayout(this);
  auto *title = new QLabel(QStringLiteral("\U0001f50d \u597d\u53cb\u7ba1\u7406\u5668"), this);
  auto *friend_notice = CreateNotificationRow(QStringLiteral("\u597d\u53cb\u901a\u77e5"));
  auto *group_notice = CreateNotificationRow(QStringLiteral("\u7fa4\u901a\u77e5"));
  auto *tabs = new QTabWidget(this);

  friend_tab_ = new FriendTabWidget(tabs);
  group_tab_ = new GroupTabWidget(tabs);
  request_panel_ = new QWidget(this);
  request_layout_ = new QVBoxLayout(request_panel_);

  title->setObjectName("paneTitle");
  request_panel_->setObjectName("friendRequestPanel");
  request_layout_->setContentsMargins(12, 8, 12, 8);
  request_layout_->setSpacing(8);
  request_panel_->hide();
  tabs->addTab(friend_tab_, QStringLiteral("\u597d\u53cb"));
  tabs->addTab(group_tab_, QStringLiteral("\u7fa4\u804a"));

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);
  root_layout->addWidget(title);
  root_layout->addWidget(friend_notice);
  root_layout->addWidget(group_notice);
  root_layout->addWidget(request_panel_);
  root_layout->addWidget(tabs, 1);

  connect(friend_tab_, &FriendTabWidget::friendMessageRequested, this,
          &FriendManagerWidget::friendMessageRequested);
  connect(group_tab_, &GroupTabWidget::groupOpenRequested, this,
          &FriendManagerWidget::groupOpenRequested);
  connect(group_tab_, &GroupTabWidget::groupsChanged, this,
          &FriendManagerWidget::dataChanged);
}

QWidget *FriendManagerWidget::CreateNotificationRow(const QString &title) {
  auto *row = new ClickableRow(this);
  auto *layout = new QHBoxLayout(row);
  auto *label = new QLabel(title, row);
  auto *arrow = new QLabel(">", row);
  row->setObjectName("groupHeader");
  layout->setContentsMargins(14, 9, 14, 9);
  layout->addWidget(label);
  layout->addStretch();
  layout->addWidget(arrow);
  row->setCursor(Qt::PointingHandCursor);
  if (title.contains(QStringLiteral("\u597d\u53cb"))) {
    row->on_click = [this]() { loadNotifications(); };
  } else {
    row->on_click = [this]() {
      request_panel_->setVisible(true);
      while (request_layout_->count() > 0) {
        QLayoutItem *item = request_layout_->takeAt(0);
        if (item->widget() != nullptr) {
          item->widget()->deleteLater();
        }
        delete item;
      }
      request_layout_->addWidget(
          new QLabel(QStringLiteral("\u6682\u65e0\u7fa4\u901a\u77e5"),
                     request_panel_));
    };
    row->setToolTip(QStringLiteral("\u6682\u65e0\u7fa4\u901a\u77e5"));
  }
  return row;
}

void FriendManagerWidget::loadData() {
  friend_tab_->loadGroups();
  group_tab_->loadGroups();
  loadNotifications();
}

void FriendManagerWidget::loadNotifications() {
  ApiClient::instance()->get(
      "/api/v1/friends/requests", this,
      [this](const QJsonObject &response) {
        RenderFriendRequests(response.value("data").toArray());
      },
      [this]() { RenderFriendRequests(QJsonArray()); });
}

void FriendManagerWidget::RenderFriendRequests(const QJsonArray &requests) {
  while (request_layout_->count() > 0) {
    QLayoutItem *item = request_layout_->takeAt(0);
    if (item->widget() != nullptr) {
      item->widget()->deleteLater();
    }
    delete item;
  }
  request_panel_->setVisible(true);
  if (requests.isEmpty()) {
    request_layout_->addWidget(
        new QLabel(QStringLiteral("\u6682\u65e0\u597d\u53cb\u7533\u8bf7"),
                   request_panel_));
    return;
  }

  for (const QJsonValue &value : requests) {
    const QJsonObject request = value.toObject();
    const int request_id = request.value("requestId").toInt();
    auto *row = new QWidget(request_panel_);
    auto *layout = new QHBoxLayout(row);
    auto *name_label = new QLabel(
        request.value("fromNickname").toString(
            request.value("fromUsername").toString()) +
            QStringLiteral("  \u7533\u8bf7\u52a0\u4e3a\u597d\u53cb"),
        row);
    auto *reject_button = new QPushButton(QStringLiteral("\u62d2\u7edd"), row);
    auto *accept_button = new QPushButton(QStringLiteral("\u540c\u610f"), row);
    reject_button->setObjectName("secondaryButton");
    accept_button->setObjectName("acceptButton");
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(name_label, 1);
    layout->addWidget(reject_button);
    layout->addWidget(accept_button);
    request_layout_->addWidget(row);
    connect(accept_button, &QPushButton::clicked, this,
            [this, request_id]() { HandleFriendRequest(request_id, 1); });
    connect(reject_button, &QPushButton::clicked, this,
            [this, request_id]() { HandleFriendRequest(request_id, 2); });
  }
}

void FriendManagerWidget::HandleFriendRequest(int request_id, int action) {
  QJsonObject body;
  body.insert("requestId", request_id);
  body.insert("action", action);
  ApiClient::instance()->post(
      "/api/v1/friends/handle", body, this,
      [this](const QJsonObject &) {
        friend_tab_->loadGroups();
        loadNotifications();
        emit dataChanged();
      });
}
