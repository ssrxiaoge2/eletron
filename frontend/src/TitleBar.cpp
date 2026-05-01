#include "TitleBar.h"

#include <QtGui/QMouseEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>

namespace {

QString TextBrandName() {
  return QStringLiteral("\u98de\u9e3d\u901a\u8baf");
}

}  // namespace

TitleBar::TitleBar(QWidget *parent) : QWidget(parent) {
  setObjectName("titleBar");
  setFixedHeight(50);

  auto *layout = new QHBoxLayout(this);
  auto *title_label = new QLabel(TextBrandName(), this);
  auto *minimize_button = new QToolButton(this);
  maximize_button_ = new QToolButton(this);
  auto *close_button = new QToolButton(this);

  title_label->setObjectName("titleBarLabel");
  minimize_button->setText(QStringLiteral("\u2014"));
  maximize_button_->setText(QStringLiteral("\u25a1"));
  close_button->setObjectName("btnClose");
  close_button->setText(QStringLiteral("\u00d7"));

  layout->setContentsMargins(16, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(title_label);
  layout->addStretch();
  layout->addWidget(minimize_button);
  layout->addWidget(maximize_button_);
  layout->addWidget(close_button);

  connect(minimize_button, &QToolButton::clicked, this, [this]() {
    window()->showMinimized();
  });
  connect(maximize_button_, &QToolButton::clicked, this,
          &TitleBar::ToggleMaximized);
  connect(close_button, &QToolButton::clicked, this,
          [this]() { window()->close(); });
}

void TitleBar::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    dragging_ = true;
    drag_pos_ =
        event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
  }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event) {
  if (dragging_ && !window()->isMaximized()) {
    window()->move(event->globalPosition().toPoint() - drag_pos_);
  }
}

void TitleBar::mouseReleaseEvent(QMouseEvent *) {
  dragging_ = false;
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    ToggleMaximized();
  }
}

void TitleBar::ToggleMaximized() {
  if (window()->isMaximized()) {
    window()->showNormal();
  } else {
    window()->showMaximized();
  }
  UpdateMaximizeButton();
}

void TitleBar::UpdateMaximizeButton() {
  maximize_button_->setText(window()->isMaximized() ? QStringLiteral("\u2750")
                                                    : QStringLiteral("\u25a1"));
}
