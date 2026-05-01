#pragma once

#include <QtWidgets/QDialog>

class QLabel;
class QLineEdit;

class EditProfileDialog : public QDialog {
  Q_OBJECT

 public:
  EditProfileDialog(const QString &nickname, const QString &signature,
                    const QString &avatar, const QString &gender,
                    const QString &birthday, QWidget *parent = nullptr);

 signals:
  void profileUpdated(const QString &nickname, const QString &signature,
                      const QString &gender, const QString &birthday);

 private:
  void SaveProfile();

  QString avatar_;
  QLabel *avatar_label_;
  QLineEdit *nickname_edit_;
  QLineEdit *signature_edit_;
  QLineEdit *gender_edit_;
  QLineEdit *birthday_edit_;
};
