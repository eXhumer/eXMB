#include "AppConfig.hxx"
#include "AppWindow.hxx"
#include <QApplication>

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setApplicationName(APP_NAME);
  app.setApplicationVersion(APP_VERSION);
  AppWindow window("bnPnumDqM7YlDueRSzZCDw");
  window.show();
  return app.exec();
}
