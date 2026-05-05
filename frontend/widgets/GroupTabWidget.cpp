#include "GroupTabWidget.h"

#include "../src/ApiClient.h"

#include <functional>
#include <QtCore/QJsonObject>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>

namespace {

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

GroupTabWidget::GroupTabWidget(QWidget *parent) : QWidget(parent) {
  auto *root_layout = new QVBoxLayout(this);
  auto *scroll_area = new QScrollArea(this);
  auto *container = new QWidget(scroll_area);
  content_layout_ = new QVBoxLayout(container);

  scroll_area->setWidgetResizable(true);
  scroll_area->setFrameShape(QFrame::NoFrame);
  scroll_area->setWidget(container);
  content_layout_->setContentsMargins(0, 0, 0, 0);
  content_layout_->setSpacing(0);
  content_layout_->addStretch();
  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->addWidget(scroll_area);
}

void GroupTabWidget::loadGroups() {
  ApiClient::instance()->get(
      "/api/v1/groups/classified", this,
      [this](const QJsonObject &response) {
        RenderGroups(response.value("data").toObject());
      });
}

void GroupTabWidget::RenderGroups(const QJsonObject &data) {
  while (content_layout_->count() > 1) {
    QLayoutItem *item = content_layout_->takeAt(0);
    if (item->widget() != nullptr) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  content_layout_->insertWidget(
      content_layout_->count() - 1,
      CreateGroupSection(QStringLiteral("\u6211\u521b\u5efa\u7684\u7fa4\u804a"),
                         data.value("owned").toArray(), 2));
  content_layout_->insertWidget(
      content_layout_->count() - 1,
      CreateGroupSection(QStringLiteral("\u6211\u7ba1\u7406\u7684\u7fa4\u804a"),
                         data.value("managed").toArray(), 1));
  content_layout_->insertWidget(
      content_layout_->count() - 1,
      CreateGroupSection(QStringLiteral("\u6211\u52a0\u5165\u7684\u7fa4\u804a"),
                         data.value("joined").toArray(), 0));
}

QWidget *GroupTabWidget::CreateGroupSection(const QString &title,
                                            const QJsonArray &groups,
                                            int role) {
  auto *section = new QWidget(this);
  auto *layout = new QVBoxLayout(section);
  auto *header = new HeaderWidget(section);
  auto *header_layout = new QHBoxLayout(header);
  auto *arrow_label = new QLabel(QStringLiteral("\u25bc"), header);
  auto *title_label =
      new QLabel(QStringLiteral("%1  %2").arg(title).arg(groups.size()),
                 header);
  auto *list = new QWidget(section);
  auto *list_layout = new QVBoxLayout(list);

  header->setObjectName("groupHeader");
  title_label->setObjectName("groupTitle");
  header_layout->setContentsMargins(12, 8, 12, 8);
  header_layout->addWidget(arrow_label);
  header_layout->addWidget(title_label);
  header_layout->addStretch();

  list_layout->setContentsMargins(0, 0, 0, 0);
  list_layout->setSpacing(0);
  if (groups.isEmpty()) {
    auto *empty_label = new QLabel(QStringLiteral("\u6682\u65e0"), list);
    empty_label->setObjectName("groupCount");
    empty_label->setContentsMargins(26, 10, 0, 10);
    list_layout->addWidget(empty_label);
  }
  for (const QJsonValue &value : groups) {
    list_layout->addWidget(CreateGroupItem(value.toObject(), role));
  }

  header->setCursor(Qt::PointingHandCursor);
  header->on_click = [arrow_label, list]() {
    const bool visible = list->isVisible();
    list->setVisible(!visible);
    arrow_label->setText(visible ? QStringLiteral("\u25b6")
                                 : QStringLiteral("\u25bc"));
  };
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(header);
  layout->addWidget(list);
  return section;
}

QWidget *GroupTabWidget::CreateGroupItem(const QJsonObject &group, int role) {
  const QString name = group.value("name").toString();
  auto *item = new QWidget(this);
  auto *layout = new QHBoxLayout(item);
  auto *avatar = new QLabel(name.left(1).toUpper(), item);
  auto *text_layout = new QVBoxLayout();
  auto *name_label = new QLabel(name, item);
  auto *count_label =
      new QLabel(QStringLiteral("%1 \u4eba")
                     .arg(group.value("memberCount").toInt()),
                 item);

  item->setObjectName("friendItem");
  item->setFixedHeight(56);
  item->setContextMenuPolicy(Qt::CustomContextMenu);
  avatar->setFixedSize(40, 40);
  avatar->setAlignment(Qt::AlignCenter);
  avatar->setStyleSheet(
      "background:#7c5cbf;color:#ffffff;border-radius:20px;font-weight:700;");
  name_label->setObjectName("friendName");
  count_label->setObjectName("friendSignature");
  text_layout->setContentsMargins(0, 0, 0, 0);
  text_layout->addWidget(name_label);
  text_layout->addWidget(count_label);
  layout->setContentsMargins(14, 6, 10, 6);
  layout->setSpacing(10);
  layout->addWidget(avatar);
  layout->addLayout(text_layout, 1);

  connect(item, &QWidget::customContextMenuRequested, this,
          [this, group, role, item]() { ShowGroupMenu(group, role, item); });
  return item;
}

void GroupTabWidget::ShowGroupMenu(const QJsonObject &group, int role,
                                   QWidget *anchor) {
  QMenu menu(this);
  QAction *open_action =
      menu.addAction(QStringLiteral("\u6253\u5f00\u7fa4\u804a"));
  QAction *danger_action = menu.addAction(
      role == 2 ? QStringLiteral("\u89e3\u6563\u8be5\u7fa4")
                : QStringLiteral("\u9000\u51fa\u8be5\u7fa4"));
  QAction *selected =
      menu.exec(anchor->mapToGlobal(QPoint(0, anchor->height())));
  if (selected == open_action) {
    emit groupOpenRequested(group.value("groupId").toInt(),
                            group.value("name").toString(),
                            group.value("memberCount").toInt(), role);
  } else if (selected == danger_action) {
    if (role == 2) {
      DeleteGroup(group.value("groupId").toInt());
    } else {
      LeaveGroup(group.value("groupId").toInt());
    }
  }
}

void GroupTabWidget::LeaveGroup(int group_id) {
  const QMessageBox::StandardButton choice = QMessageBox::question(
      this, QStringLiteral("\u9000\u51fa\u7fa4\u804a"),
      QStringLiteral("\u786e\u5b9a\u9000\u51fa\u8be5\u7fa4\u804a\uff1f"));
  if (choice != QMessageBox::Yes) {
    return;
  }
  ApiClient::instance()->deleteResource(
      QStringLiteral("/api/v1/groups/%1/members/self").arg(group_id), this,
      [this](const QJsonObject &) {
        loadGroups();
        emit groupsChanged();
      });
}

void GroupTabWidget::DeleteGroup(int group_id) {
  const QMessageBox::StandardButton choice = QMessageBox::question(
      this, QStringLiteral("\u89e3\u6563\u7fa4\u804a"),
      QStringLiteral("\u89e3\u6563\u540e\u6240\u6709\u6210\u5458\u5c06\u88ab\u79fb\u51fa\u7fa4\u804a\uff0c\u4e14\u65e0\u6cd5\u6062\u590d\uff0c\u786e\u8ba4\u89e3\u6563\uff1f"));
  if (choice != QMessageBox::Yes) {
    return;
  }
  ApiClient::instance()->deleteResource(
      QStringLiteral("/api/v1/groups/%1").arg(group_id), this,
      [this](const QJsonObject &) {
        loadGroups();
        emit groupsChanged();
      });
}
