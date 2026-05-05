#pragma once

#include <QtWidgets/QDialog>

class QButtonGroup;
class QLineEdit;
class QPushButton;

class MuteDialog : public QDialog {
  Q_OBJECT

 public:
  explicit MuteDialog(const QString &display_name, bool is_muted,
                      QWidget *parent = nullptr);
  int durationSeconds() const;

 private:
  void UpdateState();

  QButtonGroup *button_group_;
  QLineEdit *custom_minutes_edit_;
  QPushButton *ok_button_;
};
