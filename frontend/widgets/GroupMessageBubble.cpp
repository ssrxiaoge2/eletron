#include "GroupMessageBubble.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

namespace {

QString RolePrefix(int role) {
  if (role == 2) {
    return QStringLiteral("\U0001f451 ");
  }
  if (role == 1) {
    return QStringLiteral("\U0001f527 ");
  }
  return QString();
}

}  // namespace

GroupMessageBubble::GroupMessageBubble(const QString &content,
                                       MessageBubble::BubbleType type,
                                       bool is_self,
                                       const QString &sender_nickname,
                                       int sender_role, const QString &time,
                                       QWidget *parent)
    : QWidget(parent) {
  auto *root_layout = new QVBoxLayout(this);
  bubble_ = new MessageBubble(content, type, is_self, time, this);

  setAttribute(Qt::WA_TranslucentBackground, true);
  setStyleSheet("background: transparent;");
  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);

  if (!is_self) {
    auto *name_row = new QWidget(this);
    auto *name_layout = new QHBoxLayout(name_row);
    auto *name_label =
        new QLabel(RolePrefix(sender_role) + sender_nickname, name_row);
    name_label->setObjectName("groupSenderName");
    name_layout->setContentsMargins(62, 0, 18, 0);
    name_layout->addWidget(name_label);
    name_layout->addStretch();
    root_layout->addWidget(name_row);
  }
  root_layout->addWidget(bubble_);

  connect(bubble_, &MessageBubble::imageOpenRequested, this,
          &GroupMessageBubble::imageOpenRequested);
  connect(bubble_, &MessageBubble::fileDownloadRequested, this,
          &GroupMessageBubble::fileDownloadRequested);
}

void GroupMessageBubble::setMaxBubbleWidth(int max_width) {
  if (bubble_ != nullptr) {
    bubble_->setMaxBubbleWidth(max_width);
  }
}
