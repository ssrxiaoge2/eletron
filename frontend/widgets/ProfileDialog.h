#pragma once

#include <QtWidgets/QDialog>

class QLabel;

class ProfileDialog : public QDialog {
  Q_OBJECT

 public:
  ProfileDialog(const QString &username, const QString &nickname,
                const QString &signature, const QString &avatar,
                const QString &gender, const QString &birthday,
                const QString &created_at, const QString &email,
                QWidget *parent = nullptr);

 signals:
  void profileUpdated(const QString &nickname, const QString &signature,
                      const QString &gender, const QString &birthday,
                      const QString &email);

 private:
  void OpenEditor();
  void RefreshText();

  QString username_;
  QString nickname_;
  QString signature_;
  QString avatar_;
  QString gender_;
  QString birthday_;
  QString created_at_;
  QString email_;
  QLabel *avatar_label_;
  QLabel *name_label_;
  QLabel *username_label_;
  QLabel *signature_label_;
  QLabel *gender_label_;
  QLabel *birthday_label_;
  QLabel *email_label_;
  QLabel *created_at_label_;
};
