#include "MessageBubble.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>

MessageBubble::MessageBubble(const QString &text, bool sent_by_me,
                             QWidget *parent)
    : QWidget(parent) {
  auto *root_layout = new QHBoxLayout(this);
  auto *avatar = new QLabel(sent_by_me ? QStringLiteral("\u6211")
                                      : QStringLiteral("\u53cb"),
                            this);
  bubble_ = new QLabel(text, this);

  setObjectName(sent_by_me ? "sentMessageRow" : "receivedMessageRow");
  setAttribute(Qt::WA_TranslucentBackground, true);
  setStyleSheet(QStringLiteral("QWidget#%1 { background: transparent; }")
                    .arg(objectName()));
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  avatar->setObjectName("messageAvatar");
  bubble_->setObjectName(sent_by_me ? "sentBubble" : "receivedBubble");
  avatar->setAlignment(Qt::AlignCenter);
  avatar->setFixedSize(36, 36);
  bubble_->setWordWrap(true);
  bubble_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
  bubble_->setTextInteractionFlags(Qt::TextSelectableByMouse);

  root_layout->setContentsMargins(18, 6, 18, 6);
  root_layout->setSpacing(8);

  if (sent_by_me) {
    root_layout->addStretch();
    root_layout->addWidget(bubble_);
    root_layout->addWidget(avatar);
  } else {
    root_layout->addWidget(avatar);
    root_layout->addWidget(bubble_);
    root_layout->addStretch();
  }
}

void MessageBubble::setMaxBubbleWidth(int max_width) {
  bubble_->setMaximumWidth(max_width);
}
