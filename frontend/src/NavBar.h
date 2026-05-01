#pragma once

#include <QtWidgets/QWidget>

class QToolButton;

class NavBar : public QWidget {
  Q_OBJECT

 public:
  explicit NavBar(QWidget *parent = nullptr);

 private:
  QToolButton *CreateNavButton(const QString &text, bool checked);
};
