#pragma once

#include <QtWidgets/QDialog>

class QLabel;
class QDateEdit;
class QLineEdit;

class EditProfileDialog : public QDialog {
  Q_OBJECT

 public:
  EditProfileDialog(const QString &nickname, const QString &signature,
                    const QString &avatar, const QString &gender,
                    const QString &birthday, const QString &email,
                    const QString &region, QWidget *parent = nullptr);

 signals:
  void profileUpdated(const QString &nickname, const QString &signature,
                      const QString &avatar, const QString &gender,
                      const QString &birthday, const QString &email,
                      const QString &region);

 private:
  void ChooseAvatar();
  void SaveProfile();
  void UpdateAvatarPreview();

  QString avatar_;
  QLabel *avatar_label_;
  QLineEdit *nickname_edit_;
  QLineEdit *signature_edit_;
  QLineEdit *gender_edit_;
  QDateEdit *birthday_edit_;
  QLineEdit *email_edit_;
  QLineEdit *region_edit_;
};
