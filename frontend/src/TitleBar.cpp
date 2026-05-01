#include "TitleBar.h"

#include "../widgets/ProfileDialog.h"

#include <QtCore/QEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPixmap>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>

namespace {

QString TextBrandName() {
  return QStringLiteral("\u98de\u9e3d\u901a\u8baf");
}

void ApplyAvatar(QLabel *label, const QString &avatar, const QString &fallback) {
  QPixmap pixmap;
  QByteArray decoded = QByteArray::fromBase64(avatar.toUtf8());
  if (!avatar.isEmpty()) {
    if (!pixmap.loadFromData(decoded)) {
      pixmap.load(avatar);
    }
  }
  if (!pixmap.isNull()) {
    label->setPixmap(pixmap.scaled(label->size(), Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation));
    return;
  }
  label->setPixmap(QPixmap());
  label->setText(fallback);
}

}  // namespace

TitleBar::TitleBar(QWidget *parent) : QWidget(parent) {
  setObjectName("titleBar");
  setFixedHeight(50);

  auto *layout = new QHBoxLayout(this);
  auto *minimize_button = new QToolButton(this);
  maximize_button_ = new QToolButton(this);
  auto *close_button = new QToolButton(this);

  avatar_label_ = new QLabel(this);
  info_label_ = new QLabel(this);
  avatar_label_->setObjectName("titleAvatar");
  info_label_->setObjectName("titleUserInfo");
  avatar_label_->setFixedSize(40, 40);
  avatar_label_->setAlignment(Qt::AlignCenter);
  avatar_label_->setContextMenuPolicy(Qt::CustomContextMenu);
  avatar_label_->installEventFilter(this);
  info_label_->setTextFormat(Qt::RichText);

  minimize_button->setText(QStringLiteral("\u2014"));
  maximize_button_->setText(QStringLiteral("\u25a1"));
  close_button->setObjectName("btnClose");
  close_button->setText(QStringLiteral("\u00d7"));

  layout->setContentsMargins(16, 0, 0, 0);
  layout->setSpacing(10);
  layout->addWidget(avatar_label_);
  layout->addWidget(info_label_);
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
  connect(avatar_label_, &QWidget::customContextMenuRequested, this,
          [this](const QPoint &) { ShowProfileDialog(); });
  setUserInfo(QString(), QString(), QString(), QString());
}

void TitleBar::setUserInfo(const QString &username, const QString &nickname,
                           const QString &signature, const QString &avatar,
                           const QString &gender, const QString &birthday,
                           const QString &created_at, const QString &email) {
  username_ = username;
  nickname_ = nickname.isEmpty() ? username : nickname;
  signature_ = signature;
  avatar_ = avatar;
  gender_ = gender;
  birthday_ = birthday;
  created_at_ = created_at;
  email_ = email;
  const QString avatar_text =
      nickname_.isEmpty() ? QStringLiteral("\u9e3d") : nickname_.left(1).toUpper();
  ApplyAvatar(avatar_label_, avatar_, avatar_text);
  const QString short_signature =
      signature_.size() > 20 ? signature_.left(20) + "..." : signature_;
  info_label_->setText(
      QStringLiteral("<b style='font-size:14px;color:white;'>%1</b> "
                     "<span style='font-size:13px;color:#aaaaaa;'>%2</span> "
                     "<span style='font-size:12px;color:#888888;'>%3</span>")
          .arg(TextBrandName(), nickname_, short_signature));
}

bool TitleBar::eventFilter(QObject *watched, QEvent *event) {
  if (watched == avatar_label_ && event->type() == QEvent::MouseButtonRelease) {
    auto *mouse_event = static_cast<QMouseEvent *>(event);
    if (mouse_event->button() == Qt::LeftButton) {
      ShowProfileDialog();
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
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

void TitleBar::ShowProfileDialog() {
  auto *dialog = new ProfileDialog(username_, nickname_, signature_, avatar_,
                                   gender_, birthday_, created_at_, email_,
                                   this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  connect(dialog, &ProfileDialog::profileUpdated, this,
          [this](const QString &nickname, const QString &signature,
                 const QString &gender, const QString &birthday,
                 const QString &email) {
            setUserInfo(username_, nickname, signature, avatar_, gender,
                        birthday, created_at_, email);
            emit profileUpdated(nickname, signature);
          });
  dialog->move(avatar_label_->mapToGlobal(avatar_label_->rect().bottomLeft()));
  dialog->show();
}
