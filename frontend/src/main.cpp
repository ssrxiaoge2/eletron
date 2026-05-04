#include "MainWindow.h"
#include "LoginWindow.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  LoginWindow login_window;
  MainWindow main_window;

  QObject::connect(&login_window, &LoginWindow::loginSucceeded, [&]() {
    main_window.initializeSession();
    main_window.show();
    login_window.close();
  });

  login_window.show();
  return app.exec();
}
