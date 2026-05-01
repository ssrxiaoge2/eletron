#include "NavBar.h"

#include <QtWidgets/QStyle>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

NavBar::NavBar(QWidget *parent) : QWidget(parent) {
  setObjectName("navBar");
  setFixedWidth(60);

  auto *layout = new QVBoxLayout(this);
  auto *message_button =
      CreateNavButton(QStringLiteral("\u6d88"), true);
  auto *contact_button =
      CreateNavButton(QStringLiteral("\u4eba"), false);
  auto *settings_button =
      CreateNavButton(QStringLiteral("\u8bbe"), false);

  message_button->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
  contact_button->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
  settings_button->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));

  layout->setContentsMargins(10, 12, 10, 12);
  layout->setSpacing(10);
  layout->addStretch();
  layout->addWidget(message_button);
  layout->addWidget(contact_button);
  layout->addStretch();
  layout->addWidget(settings_button);
}

QToolButton *NavBar::CreateNavButton(const QString &text, bool checked) {
  auto *button = new QToolButton(this);
  button->setObjectName("navButton");
  button->setText(text);
  button->setCheckable(true);
  button->setChecked(checked);
  button->setIconSize({20, 20});
  button->setFixedSize(40, 40);
  return button;
}
