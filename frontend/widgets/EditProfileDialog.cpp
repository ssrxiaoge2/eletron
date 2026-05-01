#include "EditProfileDialog.h"

#include "../src/ApiClient.h"

#include <QtCore/QDate>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtGui/QPixmap>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace {

QString TextSaveFailed() {
  return QStringLiteral("\u4fdd\u5b58\u5931\u8d25");
}

}  // namespace

EditProfileDialog::EditProfileDialog(const QString &nickname,
                                     const QString &signature,
                                     const QString &avatar,
                                     const QString &gender,
                                     const QString &birthday,
                                     const QString &email, QWidget *parent)
    : QDialog(parent), avatar_(avatar) {
  setObjectName("darkDialog");
  setWindowTitle(QStringLiteral("\u7f16\u8f91\u8d44\u6599"));
  setModal(true);
  setFixedWidth(360);

  auto *root_layout = new QVBoxLayout(this);
  auto *form_layout = new QFormLayout();
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save |
                                           QDialogButtonBox::Cancel,
                                       this);

  avatar_label_ = new QLabel(this);
  nickname_edit_ = new QLineEdit(nickname, this);
  signature_edit_ = new QLineEdit(signature, this);
  gender_edit_ = new QLineEdit(gender, this);
  birthday_edit_ = new QDateEdit(this);
  email_edit_ = new QLineEdit(email, this);

  avatar_label_->setObjectName("profileAvatarLarge");
  avatar_label_->setFixedSize(80, 80);
  avatar_label_->setAlignment(Qt::AlignCenter);
  UpdateAvatarPreview();
  auto *choose_avatar_button =
      new QPushButton(QStringLiteral("\u66f4\u6362\u5934\u50cf"), this);

  birthday_edit_->setCalendarPopup(true);
  birthday_edit_->setDisplayFormat("yyyy-MM-dd");
  const QDate parsed_birthday = QDate::fromString(birthday, "yyyy-MM-dd");
  birthday_edit_->setDate(parsed_birthday.isValid() ? parsed_birthday
                                                    : QDate(2000, 1, 1));

  form_layout->addRow(QStringLiteral("\u6635\u79f0"), nickname_edit_);
  form_layout->addRow(QStringLiteral("\u4e2a\u6027\u7b7e\u540d"),
                      signature_edit_);
  form_layout->addRow(QStringLiteral("\u6027\u522b"), gender_edit_);
  form_layout->addRow(QStringLiteral("\u751f\u65e5"), birthday_edit_);
  form_layout->addRow(QStringLiteral("\u90ae\u7bb1"), email_edit_);

  root_layout->setContentsMargins(20, 18, 20, 18);
  root_layout->setSpacing(14);
  root_layout->addWidget(avatar_label_, 0, Qt::AlignHCenter);
  root_layout->addWidget(choose_avatar_button, 0, Qt::AlignHCenter);
  root_layout->addLayout(form_layout);
  root_layout->addWidget(buttons);

  connect(choose_avatar_button, &QPushButton::clicked, this,
          &EditProfileDialog::ChooseAvatar);
  connect(buttons->button(QDialogButtonBox::Save), &QPushButton::clicked, this,
          &EditProfileDialog::SaveProfile);
  connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
          this, &QDialog::reject);
}

void EditProfileDialog::ChooseAvatar() {
  const QString file_path = QFileDialog::getOpenFileName(
      this, QStringLiteral("\u9009\u62e9\u5934\u50cf"), QString(),
      QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp)"));
  if (file_path.isEmpty()) {
    return;
  }

  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, QString(), TextSaveFailed());
    return;
  }
  const QByteArray image_bytes = file.readAll();
  avatar_ = QString::fromLatin1(image_bytes.toBase64());
  UpdateAvatarPreview();
}

void EditProfileDialog::SaveProfile() {
  QJsonObject body;
  body.insert("nickname", nickname_edit_->text().trimmed());
  body.insert("signature", signature_edit_->text().trimmed());
  body.insert("avatar", avatar_);
  body.insert("gender", gender_edit_->text().trimmed());
  body.insert("birthday", birthday_edit_->date().toString("yyyy-MM-dd"));
  body.insert("email", email_edit_->text().trimmed());

  ApiClient::instance()->put(
      "/api/v1/user/profile", body, this,
      [this](const QJsonObject &response) {
        const QJsonObject data = response.value("data").toObject();
        const QString nickname = data.value("nickname").toString(
            nickname_edit_->text().trimmed());
        const QString signature = data.value("signature").toString(
            signature_edit_->text().trimmed());
        const QString gender =
            data.value("gender").toString(gender_edit_->text().trimmed());
        const QString birthday =
            data.value("birthday").toString(
                birthday_edit_->date().toString("yyyy-MM-dd"));
        const QString email =
            data.value("email").toString(email_edit_->text().trimmed());
        const QString avatar = data.value("avatar").toString(avatar_);
        emit profileUpdated(nickname, signature, avatar, gender, birthday, email);
        accept();
      },
      [this]() { QMessageBox::warning(this, QString(), TextSaveFailed()); });
}

void EditProfileDialog::UpdateAvatarPreview() {
  QPixmap avatar_pixmap;
  if (!avatar_.isEmpty() &&
      !avatar_pixmap.loadFromData(QByteArray::fromBase64(avatar_.toUtf8()))) {
    avatar_pixmap.load(avatar_);
  }
  if (avatar_pixmap.isNull()) {
    avatar_label_->setPixmap(QPixmap());
    avatar_label_->setText(nickname_edit_ == nullptr ||
                                   nickname_edit_->text().trimmed().isEmpty()
                               ? QStringLiteral("\u9e3d")
                               : nickname_edit_->text().trimmed().left(1).toUpper());
    return;
  }
  avatar_label_->setText(QString());
  avatar_label_->setPixmap(avatar_pixmap.scaled(
      avatar_label_->size(), Qt::KeepAspectRatioByExpanding,
      Qt::SmoothTransformation));
}
