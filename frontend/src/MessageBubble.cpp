#include "MessageBubble.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

MessageBubble::MessageBubble(const QString &text, bool sent_by_me,
                             QWidget *parent)
    : QWidget(parent) {
  auto *root_layout = new QHBoxLayout(this);
  auto *avatar = new QLabel(sent_by_me ? QStringLiteral("\u6211")
                                      : QStringLiteral("\u53cb"),
                            this);
  auto *bubble = new QLabel(text, this);

  setObjectName(sent_by_me ? "sentMessageRow" : "receivedMessageRow");
  setAttribute(Qt::WA_StyledBackground, true);
  avatar->setObjectName("messageAvatar");
  bubble->setObjectName(sent_by_me ? "sentBubble" : "receivedBubble");
  avatar->setAlignment(Qt::AlignCenter);
  avatar->setFixedSize(36, 36);
  bubble->setWordWrap(true);
  bubble->setMaximumWidth(520);
  bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);

  root_layout->setContentsMargins(18, 6, 18, 6);
  root_layout->setSpacing(8);

  if (sent_by_me) {
    root_layout->addStretch();
    root_layout->addWidget(bubble);
    root_layout->addWidget(avatar);
  } else {
    root_layout->addWidget(avatar);
    root_layout->addWidget(bubble);
    root_layout->addStretch();
  }
}
