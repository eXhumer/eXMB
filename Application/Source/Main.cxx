/*
 * eXMB - Reddit media poster via external media hosting platforms
 * Copyright (C) 2022 - eXhumer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "AppConfig.hxx"
#include "AppWindow.hxx"
#include <QApplication>

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QIcon icon(":/AppLogo.png");
  app.setWindowIcon(icon);
  app.setApplicationName(APP_NAME);
  app.setOrganizationName(APP_ORGANIZATION);
  app.setApplicationVersion(APP_VERSION);
  AppWindow window("bnPnumDqM7YlDueRSzZCDw");
  window.show();
  return app.exec();
}
