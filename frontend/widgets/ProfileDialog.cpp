#include "ProfileDialog.h"

#include "EditProfileDialog.h"

#include <QtCore/QByteArray>
#include <QtGui/QPixmap>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace {

void ApplyAvatar(QLabel *label, const QString &avatar, const QString &fallback) {
  QPixmap pixmap;
  if (!avatar.isEmpty()) {
    if (!pixmap.loadFromData(QByteArray::fromBase64(avatar.toUtf8()))) {
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

ProfileDialog::ProfileDialog(const QString &username, const QString &nickname,
                             const QString &signature, const QString &avatar,
                             const QString &gender, const QString &birthday,
                             const QString &created_at, const QString &email,
                             QWidget *parent)
    : QDialog(parent),
      username_(username),
      nickname_(nickname),
      signature_(signature),
      avatar_(avatar),
      gender_(gender),
      birthday_(birthday),
      created_at_(created_at),
      email_(email) {
  setObjectName("profileDialog");
  setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
  setFixedWidth(320);

  auto *root_layout = new QVBoxLayout(this);
  auto *divider = new QFrame(this);
  auto *button_layout = new QHBoxLayout();
  auto *edit_button = new QPushButton(QStringLiteral("\u7f16\u8f91\u8d44\u6599"),
                                      this);
  auto *close_button = new QPushButton(QStringLiteral("\u5173\u95ed"), this);

  avatar_label_ = new QLabel(this);
  name_label_ = new QLabel(this);
  username_label_ = new QLabel(this);
  signature_label_ = new QLabel(this);
  gender_label_ = new QLabel(this);
  birthday_label_ = new QLabel(this);
  email_label_ = new QLabel(this);
  created_at_label_ = new QLabel(this);

  avatar_label_->setObjectName("profileAvatarLarge");
  name_label_->setObjectName("profileName");
  avatar_label_->setFixedSize(80, 80);
  avatar_label_->setAlignment(Qt::AlignCenter);
  divider->setFrameShape(QFrame::HLine);
  divider->setObjectName("profileDivider");

  button_layout->addStretch();
  button_layout->addWidget(edit_button);
  button_layout->addWidget(close_button);

  root_layout->setContentsMargins(20, 18, 20, 18);
  root_layout->setSpacing(10);
  root_layout->addWidget(avatar_label_, 0, Qt::AlignHCenter);
  root_layout->addWidget(name_label_);
  root_layout->addWidget(username_label_);
  root_layout->addWidget(signature_label_);
  root_layout->addWidget(gender_label_);
  root_layout->addWidget(birthday_label_);
  root_layout->addWidget(email_label_);
  root_layout->addWidget(created_at_label_);
  root_layout->addWidget(divider);
  root_layout->addLayout(button_layout);

  RefreshText();

  connect(edit_button, &QPushButton::clicked, this, &ProfileDialog::OpenEditor);
  connect(close_button, &QPushButton::clicked, this, &QDialog::close);
}

void ProfileDialog::OpenEditor() {
  EditProfileDialog dialog(nickname_, signature_, avatar_, gender_, birthday_,
                           email_, this);
  connect(&dialog, &EditProfileDialog::profileUpdated, this,
          [this](const QString &nickname, const QString &signature,
                 const QString &gender, const QString &birthday,
                 const QString &email) {
            nickname_ = nickname;
            signature_ = signature;
            gender_ = gender;
            birthday_ = birthday;
            email_ = email;
            RefreshText();
            emit profileUpdated(nickname_, signature_, gender_, birthday_,
                                email_);
          });
  dialog.exec();
}

void ProfileDialog::RefreshText() {
  const QString display_name = nickname_.isEmpty() ? username_ : nickname_;
  ApplyAvatar(avatar_label_, avatar_, display_name.isEmpty()
                                       ? QStringLiteral("\u9e3d")
                                       : display_name.left(1).toUpper());
  name_label_->setText(display_name);
  username_label_->setText(QStringLiteral("\u7528\u6237\u540d\uff1a%1")
                               .arg(username_));
  signature_label_->setText(QStringLiteral("\u4e2a\u6027\u7b7e\u540d\uff1a%1")
                                .arg(signature_));
  gender_label_->setText(QStringLiteral("\u6027\u522b\uff1a%1").arg(gender_));
  birthday_label_->setText(QStringLiteral("\u751f\u65e5\uff1a%1").arg(birthday_));
  email_label_->setText(QStringLiteral("\u90ae\u7bb1\uff1a%1").arg(email_));
  created_at_label_->setText(QStringLiteral("\u6ce8\u518c\u65f6\u95f4\uff1a%1")
                                 .arg(created_at_));
}
