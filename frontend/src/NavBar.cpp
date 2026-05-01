#include "NavBar.h"

#include <QtWidgets/QStyle>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

NavBar::NavBar(QWidget *parent) : QWidget(parent) {
  setObjectName("navBar");
  setFixedWidth(60);

  auto *layout = new QVBoxLayout(this);
  message_button_ = CreateNavButton(QStringLiteral("\u6d88"), true, 0);
  friend_button_ = CreateNavButton(QStringLiteral("\u4eba"), false, 1);
  auto *settings_button =
      CreateNavButton(QStringLiteral("\u8bbe"), false, 2);

  message_button_->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
  friend_button_->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
  settings_button->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));

  layout->setContentsMargins(10, 12, 10, 12);
  layout->setSpacing(10);
  layout->addStretch();
  layout->addWidget(message_button_);
  layout->addWidget(friend_button_);
  layout->addStretch();
  layout->addWidget(settings_button);
}

void NavBar::setCurrentIndex(int index) {
  message_button_->setChecked(index == 0);
  friend_button_->setChecked(index == 1);
}

void NavBar::setFriendBadge(int count) {
  friend_button_->setText(count > 0 ? QString::number(count)
                                    : QStringLiteral("\u4eba"));
}

QToolButton *NavBar::CreateNavButton(const QString &text, bool checked,
                                     int index) {
  auto *button = new QToolButton(this);
  button->setObjectName("navButton");
  button->setText(text);
  button->setCheckable(true);
  button->setChecked(checked);
  button->setIconSize({20, 20});
  button->setFixedSize(40, 40);
  connect(button, &QToolButton::clicked, this, [this, index]() {
    if (index <= 1) {
      setCurrentIndex(index);
      emit currentIndexChanged(index);
    }
  });
  return button;
}
