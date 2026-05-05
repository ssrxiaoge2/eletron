#include "MuteDialog.h"

#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>

MuteDialog::MuteDialog(const QString &display_name, bool is_muted,
                       QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(QStringLiteral("\u7981\u8a00"));
  setModal(true);
  setFixedWidth(320);

  auto *root_layout = new QVBoxLayout(this);
  auto *title =
      new QLabel(QStringLiteral("\u7981\u8a00 - %1").arg(display_name), this);
  auto *custom_row = new QWidget(this);
  auto *custom_layout = new QHBoxLayout(custom_row);
  auto *custom_radio =
      new QRadioButton(QStringLiteral("\u81ea\u5b9a\u4e49"), custom_row);
  auto *minutes_label = new QLabel(QStringLiteral("\u5206\u949f"), custom_row);
  auto *button_row = new QWidget(this);
  auto *button_layout = new QHBoxLayout(button_row);
  auto *cancel_button =
      new QPushButton(QStringLiteral("\u53d6\u6d88"), button_row);

  button_group_ = new QButtonGroup(this);
  custom_minutes_edit_ = new QLineEdit(custom_row);
  ok_button_ = new QPushButton(QStringLiteral("\u786e\u8ba4"), button_row);

  const QList<QPair<QString, int>> options = {
      {QStringLiteral("1 \u5206\u949f"), 60},
      {QStringLiteral("5 \u5206\u949f"), 300},
      {QStringLiteral("10 \u5206\u949f"), 600},
      {QStringLiteral("1 \u5c0f\u65f6"), 3600},
      {QStringLiteral("24 \u5c0f\u65f6"), 86400},
  };

  root_layout->setContentsMargins(22, 18, 22, 18);
  root_layout->setSpacing(10);
  root_layout->addWidget(title);
  for (const auto &option : options) {
    auto *radio = new QRadioButton(option.first, this);
    button_group_->addButton(radio, option.second);
    root_layout->addWidget(radio);
    if (option.second == 300) {
      radio->setChecked(true);
    }
  }
  if (is_muted) {
    auto *unmute_radio =
        new QRadioButton(QStringLiteral("\u89e3\u9664\u7981\u8a00"), this);
    button_group_->addButton(unmute_radio, 0);
    root_layout->addWidget(unmute_radio);
  }

  custom_minutes_edit_->setPlaceholderText(QStringLiteral("30"));
  custom_minutes_edit_->setFixedWidth(80);
  button_group_->addButton(custom_radio, -1);
  custom_layout->setContentsMargins(0, 0, 0, 0);
  custom_layout->addWidget(custom_radio);
  custom_layout->addWidget(custom_minutes_edit_);
  custom_layout->addWidget(minutes_label);
  custom_layout->addStretch();
  root_layout->addWidget(custom_row);

  button_layout->setContentsMargins(0, 0, 0, 0);
  button_layout->addStretch();
  button_layout->addWidget(cancel_button);
  button_layout->addWidget(ok_button_);
  root_layout->addWidget(button_row);

  connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);
  connect(ok_button_, &QPushButton::clicked, this, &QDialog::accept);
  connect(button_group_, &QButtonGroup::idClicked, this,
          &MuteDialog::UpdateState);
  connect(custom_minutes_edit_, &QLineEdit::textChanged, this,
          &MuteDialog::UpdateState);
  UpdateState();
}

int MuteDialog::durationSeconds() const {
  const int id = button_group_->checkedId();
  if (id >= 0) {
    return id;
  }
  return custom_minutes_edit_->text().toInt() * 60;
}

void MuteDialog::UpdateState() {
  custom_minutes_edit_->setEnabled(button_group_->checkedId() == -1);
  ok_button_->setEnabled(button_group_->checkedId() != -1 ||
                         custom_minutes_edit_->text().toInt() > 0);
}
