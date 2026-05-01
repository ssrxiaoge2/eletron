#include "ChatListWidget.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

namespace {

class SessionItemWidget : public QWidget {
 public:
  SessionItemWidget(const QString &name, const QString &time,
                    const QString &preview, int unread_count,
                    QWidget *parent = nullptr)
      : QWidget(parent) {
    auto *root_layout = new QHBoxLayout(this);
    auto *avatar = new QLabel(name.left(1).toUpper(), this);
    auto *text_layout = new QVBoxLayout();
    auto *top_layout = new QHBoxLayout();
    auto *name_label = new QLabel(name, this);
    auto *time_label = new QLabel(time, this);
    auto *bottom_layout = new QHBoxLayout();
    auto *preview_label = new QLabel(preview, this);
    auto *unread_label = new QLabel(QString::number(unread_count), this);

    setObjectName("sessionItem");
    avatar->setObjectName("sessionAvatar");
    name_label->setObjectName("sessionName");
    time_label->setObjectName("sessionTime");
    preview_label->setObjectName("sessionPreview");
    unread_label->setObjectName("unreadBadge");

    avatar->setFixedSize(42, 42);
    avatar->setAlignment(Qt::AlignCenter);
    preview_label->setTextFormat(Qt::PlainText);
    preview_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    unread_label->setVisible(unread_count > 0);
    unread_label->setAlignment(Qt::AlignCenter);

    top_layout->setContentsMargins(0, 0, 0, 0);
    top_layout->addWidget(name_label);
    top_layout->addStretch();
    top_layout->addWidget(time_label);

    bottom_layout->setContentsMargins(0, 0, 0, 0);
    bottom_layout->addWidget(preview_label);
    bottom_layout->addStretch();
    bottom_layout->addWidget(unread_label);

    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(4);
    text_layout->addLayout(top_layout);
    text_layout->addLayout(bottom_layout);

    root_layout->setContentsMargins(12, 8, 10, 8);
    root_layout->setSpacing(10);
    root_layout->addWidget(avatar);
    root_layout->addLayout(text_layout, 1);
  }
};

}  // namespace

ChatListWidget::ChatListWidget(QWidget *parent) : QWidget(parent) {
  setObjectName("chatListPane");
  setMinimumWidth(240);

  auto *root_layout = new QVBoxLayout(this);
  auto *search_bar = new QWidget(this);
  auto *search_layout = new QHBoxLayout(search_bar);
  auto *search_edit = new QLineEdit(search_bar);
  auto *new_chat_button = new QToolButton(search_bar);

  session_list_ = new QListWidget(this);

  search_bar->setObjectName("searchBar");
  search_edit->setObjectName("searchEdit");
  search_edit->setPlaceholderText(QStringLiteral("\u641c\u7d22"));
  new_chat_button->setObjectName("newChatButton");
  new_chat_button->setText("+");
  new_chat_button->setFixedSize(32, 32);

  search_layout->setContentsMargins(12, 8, 12, 8);
  search_layout->setSpacing(8);
  search_layout->addWidget(search_edit);
  search_layout->addWidget(new_chat_button);

  session_list_->setObjectName("sessionList");
  session_list_->setFrameShape(QFrame::NoFrame);
  session_list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);
  root_layout->addWidget(search_bar);
  root_layout->addWidget(session_list_, 1);

  AddMockSession(QStringLiteral("\u4e1c\u65b9-Askeai"),
                 QStringLiteral("15:27"),
                 QStringLiteral("\u4eca\u5929\u665a\u4e0a\u4e00\u8d77\u804a"),
                 3);
  AddMockSession(QStringLiteral("\u98de\u9e3d\u5f00\u53d1\u7fa4"),
                 QStringLiteral("14:08"),
                 QStringLiteral("MainWindow \u5df2\u5f00\u59cb\u642d\u5efa"),
                 0);
  AddMockSession(QStringLiteral("\u6797\u5c0f\u96e8"),
                 QStringLiteral("\u6628\u5929"),
                 QStringLiteral("\u6587\u4ef6\u6536\u5230\u4e86\uff0c\u8c22\u8c22"),
                 9);
  AddMockSession(QStringLiteral("\u7cfb\u7edf\u901a\u77e5"),
                 QStringLiteral("\u5468\u4e00"),
                 QStringLiteral("\u6b22\u8fce\u4f7f\u7528\u98de\u9e3d\u901a\u8baf"),
                 0);
}

void ChatListWidget::AddMockSession(const QString &name, const QString &time,
                                    const QString &preview, int unread_count) {
  auto *item = new QListWidgetItem();
  auto *widget =
      new SessionItemWidget(name, time, preview, unread_count, session_list_);
  item->setSizeHint({300, 70});
  session_list_->addItem(item);
  session_list_->setItemWidget(item, widget);
}
