#include "GroupMessageBubble.h"

#include <QtCore/QDebug>
#include <QtWidgets/QSizePolicy>
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

QString AvatarText(const QString &nickname, bool is_self) {
  if (is_self) {
    return QStringLiteral("\u6211");
  }
  return nickname.isEmpty() ? QStringLiteral("?") : nickname.left(1).toUpper();
}

}  // namespace

GroupMessageBubble::GroupMessageBubble(const QString &content,
                                       MessageBubble::BubbleType type,
                                       bool is_self,
                                       const QString &sender_nickname,
                                       int sender_role, const QString &time,
                                       QWidget *parent)
    : QWidget(parent) {
  Q_UNUSED(type);
  Q_UNUSED(time);

  qDebug() << "\u6784\u9020GroupMessageBubble:"
           << "isSelf=" << is_self
           << "nickname=" << sender_nickname
           << "role=" << sender_role
           << "content=" << content.left(20);

  setAttribute(Qt::WA_TranslucentBackground, true);
  setStyleSheet("background: transparent;");
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  auto *outer_layout = new QHBoxLayout(this);
  auto *avatar_label =
      new QLabel(AvatarText(sender_nickname, is_self), this);
  auto *center_widget = new QWidget(this);
  auto *center_layout = new QVBoxLayout(center_widget);

  avatar_label->setFixedSize(36, 36);
  avatar_label->setAlignment(Qt::AlignCenter);
  avatar_label->setStyleSheet(
      "background: #4a90d9; color: #ffffff; border-radius: 18px; "
      "font-size: 14px; font-weight: 700;");

  center_widget->setStyleSheet("background: transparent;");
  center_layout->setContentsMargins(0, 0, 0, 0);
  center_layout->setSpacing(4);

  if (!is_self) {
    auto *name_row = new QWidget(center_widget);
    auto *name_layout = new QHBoxLayout(name_row);
    name_row->setStyleSheet("background: transparent;");
    name_layout->setContentsMargins(0, 0, 0, 0);
    name_layout->setSpacing(4);

    const QString role = RolePrefix(sender_role).trimmed();
    if (!role.isEmpty()) {
      auto *role_label = new QLabel(role, name_row);
      role_label->setStyleSheet("background: transparent; font-size: 13px;");
      name_layout->addWidget(role_label);
    }

    auto *nickname_label = new QLabel(
        sender_nickname.isEmpty() ? QStringLiteral("\u672a\u77e5\u6210\u5458")
                                  : sender_nickname,
        name_row);
    nickname_label->setObjectName("groupSenderName");
    nickname_label->setStyleSheet(
        "background: transparent; color: #888888; font-size: 12px;");
    name_layout->addWidget(nickname_label);
    name_layout->addStretch();
    center_layout->addWidget(name_row);
  }

  auto *bubble_row = new QWidget(center_widget);
  auto *bubble_row_layout = new QHBoxLayout(bubble_row);
  auto *bubble_label = new QLabel(content, bubble_row);
  bubble_ = bubble_label;

  bubble_row->setStyleSheet("background: transparent;");
  bubble_row_layout->setContentsMargins(0, 0, 0, 0);
  bubble_row_layout->setSpacing(0);
  bubble_label->setWordWrap(true);
  bubble_label->setMaximumWidth(400);
  bubble_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  bubble_label->setStyleSheet(
      is_self ? "background: #1d6fa4; color: #ffffff; border-radius: 12px; "
                "padding: 8px 12px; font-size: 14px;"
              : "background: #4a90d9; color: #ffffff; border-radius: 12px; "
                "padding: 8px 12px; font-size: 14px;");
  if (is_self) {
    bubble_row_layout->addStretch();
    bubble_row_layout->addWidget(bubble_label);
  } else {
    bubble_row_layout->addWidget(bubble_label);
    bubble_row_layout->addStretch();
  }
  center_layout->addWidget(bubble_row);

  outer_layout->setContentsMargins(8, 4, 8, 4);
  outer_layout->setSpacing(8);
  if (is_self) {
    outer_layout->addStretch();
    outer_layout->addWidget(center_widget);
    outer_layout->addWidget(avatar_label);
  } else {
    outer_layout->addWidget(avatar_label);
    outer_layout->addWidget(center_widget);
    outer_layout->addStretch();
  }
}

void GroupMessageBubble::setMaxBubbleWidth(int max_width) {
  if (bubble_ != nullptr) {
    bubble_->setMaximumWidth(max_width);
  }
}
