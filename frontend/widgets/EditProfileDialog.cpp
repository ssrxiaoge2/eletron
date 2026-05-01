#include "EditProfileDialog.h"

#include "../src/ApiClient.h"

#include <QtCore/QJsonObject>
#include <QtWidgets/QDialogButtonBox>
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
                                     const QString &birthday, QWidget *parent)
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
  birthday_edit_ = new QLineEdit(birthday, this);

  avatar_label_->setObjectName("profileAvatarLarge");
  avatar_label_->setFixedSize(80, 80);
  avatar_label_->setAlignment(Qt::AlignCenter);
  avatar_label_->setText(nickname.isEmpty() ? QStringLiteral("\u9e3d")
                                            : nickname.left(1).toUpper());

  form_layout->addRow(QStringLiteral("\u6635\u79f0"), nickname_edit_);
  form_layout->addRow(QStringLiteral("\u4e2a\u6027\u7b7e\u540d"),
                      signature_edit_);
  form_layout->addRow(QStringLiteral("\u6027\u522b"), gender_edit_);
  form_layout->addRow(QStringLiteral("\u751f\u65e5"), birthday_edit_);

  root_layout->setContentsMargins(20, 18, 20, 18);
  root_layout->setSpacing(14);
  root_layout->addWidget(avatar_label_, 0, Qt::AlignHCenter);
  root_layout->addLayout(form_layout);
  root_layout->addWidget(buttons);

  connect(buttons->button(QDialogButtonBox::Save), &QPushButton::clicked, this,
          &EditProfileDialog::SaveProfile);
  connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
          this, &QDialog::reject);
}

void EditProfileDialog::SaveProfile() {
  QJsonObject body;
  body.insert("nickname", nickname_edit_->text().trimmed());
  body.insert("signature", signature_edit_->text().trimmed());
  body.insert("avatar", avatar_);
  body.insert("gender", gender_edit_->text().trimmed());
  body.insert("birthday", birthday_edit_->text().trimmed());

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
            data.value("birthday").toString(birthday_edit_->text().trimmed());
        emit profileUpdated(nickname, signature, gender, birthday);
        accept();
      },
      [this]() { QMessageBox::warning(this, QString(), TextSaveFailed()); });
}
