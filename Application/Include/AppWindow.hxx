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

#ifndef APPWINDOW_HXX
#define APPWINDOW_HXX

namespace eXRC::Service {
class Reddit;
} // namespace eXRC::Service

namespace eXVHP::Service {
class MediaService;
} // namespace eXVHP::Service

#include <QCheckBox>
#include <QCompleter>
#include <QDir>
#include <QGroupBox>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>

class AppWindow : public QMainWindow {
  Q_OBJECT

private:
#if WIN32
  bool m_darkMode;
#endif // WIN32
  QDir m_appDataDir;
  eXRC::Service::Reddit *m_red;
  eXVHP::Service::MediaService *m_media;
  QCheckBox *m_permanentCB, *m_postNSFWCB, *m_postSRCB, *m_postSpoilerCB;
  QCompleter *m_flairCompleter;
  QGroupBox *m_authGB, *m_postGB;
  QLineEdit *m_subredditLE, *m_titleLE, *m_flairLE, *m_authStateLE;
  QProgressBar *m_uploadProgress;
  QPushButton *m_authBtn, *m_revokeBtn, *m_videoSelectBtn;
  QRadioButton *m_dubzRB, *m_imgurRB, *m_jslRB, *m_redRB, *m_sabRB, *m_sffRB,
      *m_sjaRB;
  void setupCentralWidget();
  void setupMenuBar();
  void setupServices(const QString &redditClientId);
  void setupWidgets();
  QMessageBox *CreateMessageBox(QMessageBox::Icon icon, const QString &title,
                                const QString &text,
                                QMessageBox::StandardButtons buttons =
                                    QMessageBox::StandardButton::NoButton,
                                QWidget *parent = nullptr);

public:
  AppWindow(const QString &redditClientId, QWidget *parent = nullptr);

private slots:
  void enableWidgets();
  void disableWidgets();
  void onAuthorize();
  void onReady(const QJsonObject &identity);
  void onGrantError(const QString &error);
  void onRevoke();
  void onRevoked();
  void onRevokeError(const QString &error);
  void onVideoFileSelectAndUpload();
};

#endif // APPWINDOW_HXX
