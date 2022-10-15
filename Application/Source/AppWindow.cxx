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

#include "AppWindow.hxx"
#include "AppConfig.hxx"
#include "FlairListModel.hxx"
#include "eXRC/Reddit.hxx"
#include "eXVHP/Service.hxx"
#include <QApplication>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QStandardPaths>
#include <QVBoxLayout>
#ifdef WIN32
#include <QSettings>
#include <QTimer>
#include <dwmapi.h>

COLORREF DARK_COLOR = 0x00505050;
COLORREF LIGHT_COLOR = 0x00FFFFFF;
COLORREF DARK_TEXT_COLOR = 0x009B9B9B;
COLORREF LIGHT_TEXT_COLOR = 0x00000000;

bool IsWindowsDarkMode(bool *ok = nullptr) {
  QSettings currentTheme(
      "HKEY_CURRENT_"
      "USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
      QSettings::NativeFormat);
  return currentTheme.value("AppsUseLightTheme").toUInt(ok) == 0;
}

void SetDarkModeStatus(bool useDarkMode, HWND winId) {
  BOOL USE_DARK_MODE = useDarkMode;
  DwmSetWindowAttribute(winId, DWMWINDOWATTRIBUTE::DWMWA_CAPTION_COLOR,
                        USE_DARK_MODE ? &DARK_COLOR : &LIGHT_COLOR,
                        sizeof(USE_DARK_MODE ? DARK_COLOR : LIGHT_COLOR));
  DwmSetWindowAttribute(
      winId, DWMWINDOWATTRIBUTE::DWMWA_TEXT_COLOR,
      USE_DARK_MODE ? &DARK_TEXT_COLOR : &LIGHT_TEXT_COLOR,
      sizeof(USE_DARK_MODE ? DARK_TEXT_COLOR : LIGHT_TEXT_COLOR));
  DwmSetWindowAttribute(winId,
                        DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
                        &USE_DARK_MODE, sizeof(USE_DARK_MODE));
}
#endif // WIN32

QMessageBox *AppWindow::CreateMessageBox(QMessageBox::Icon icon,
                                         const QString &title,
                                         const QString &text,
                                         QMessageBox::StandardButtons buttons,
                                         QWidget *parent) {
  QMessageBox *msgBox = new QMessageBox(icon, title, text, buttons, parent);
#ifdef WIN32
  SetDarkModeStatus(this->m_darkMode, HWND(msgBox->winId()));

  QTimer *themeChangeTimer = new QTimer(msgBox);
  bool darkMode = this->m_darkMode;
  themeChangeTimer->connect(
      themeChangeTimer, &QTimer::timeout, msgBox, [msgBox, &darkMode]() {
        bool newDarkMode = darkMode;

        if (darkMode != newDarkMode) {
          darkMode = newDarkMode;
          SetDarkModeStatus(darkMode, HWND(msgBox->winId()));
        }
      });
  themeChangeTimer->start(1000);
#endif // WIN32

  return msgBox;
}

AppWindow::AppWindow(const QString &redditClientId, QWidget *parent)
    : QMainWindow(parent), m_appDataDir(QStandardPaths::writableLocation(
                               QStandardPaths::AppDataLocation)) {
#ifdef WIN32
  m_darkMode = IsWindowsDarkMode();
  SetDarkModeStatus(m_darkMode, HWND(winId()));
#endif // WIN32
  QFile appQssFile(":/AppWindowDark.qss");
  appQssFile.open(QIODevice::ReadOnly);
  setStyleSheet(QString(appQssFile.readAll()));

  if (!m_appDataDir.exists())
    m_appDataDir.mkpath(".");

  setupWidgets();
  setupServices(redditClientId);
  setupCentralWidget();
  setupMenuBar();
}

void AppWindow::setupCentralWidget() {
  QVBoxLayout *authLayout = new QVBoxLayout;
  QHBoxLayout *authStateLayout = new QHBoxLayout;
  QHBoxLayout *postLayout = new QHBoxLayout;
  QHBoxLayout *postOptsLayout = new QHBoxLayout;
  QVBoxLayout *postHostLayout = new QVBoxLayout;
  QVBoxLayout *postDetailsLayout = new QVBoxLayout;
  QVBoxLayout *centralLayout = new QVBoxLayout;
  QWidget *centralWidget = new QWidget;

  m_subredditLE->setPlaceholderText("Post Subreddit (Optional, posts to user's "
                                    "profile if no subreddit specified)");
  m_titleLE->setPlaceholderText("Post Title");
  m_flairLE->setPlaceholderText("Post Flair ID (Optional)");

  postHostLayout->addWidget(new QLabel("Video Host"), 0, Qt::AlignHCenter);
  postHostLayout->addWidget(m_imgurRB);
  postHostLayout->addWidget(m_jslRB);
  postHostLayout->addWidget(m_redRB);
  postHostLayout->addWidget(m_sabRB);
  postHostLayout->addWidget(m_sffRB);
  postHostLayout->addWidget(m_sjaRB);
  postLayout->addLayout(postHostLayout);
  postOptsLayout->addWidget(m_postNSFWCB, 0, Qt::AlignHCenter);
  postOptsLayout->addWidget(m_postSRCB, 0, Qt::AlignHCenter);
  postOptsLayout->addWidget(m_postSpoilerCB, 0, Qt::AlignHCenter);
  postDetailsLayout->addWidget(m_titleLE);
  postDetailsLayout->addWidget(m_subredditLE);
  postDetailsLayout->addWidget(m_flairLE);
  postDetailsLayout->addWidget(m_videoSelectBtn);
  postDetailsLayout->addWidget(m_uploadProgress);
  postDetailsLayout->addLayout(postOptsLayout);
  postLayout->addLayout(postDetailsLayout, 1);
  authStateLayout->addWidget(m_authStateLE);
  authStateLayout->addWidget(m_permanentCB);
  authLayout->addLayout(authStateLayout);
  authLayout->addWidget(m_authBtn);
  authLayout->addWidget(m_revokeBtn);
  m_authGB->setLayout(authLayout);
  m_postGB->setLayout(postLayout);

  QObject::disconnect(m_flairCompleter, nullptr, nullptr, nullptr);
  connect(m_flairCompleter,
          QOverload<const QModelIndex &>::of(&QCompleter::activated),
          [this](const QModelIndex &index) {
            m_flairLE->setText(index.data(Qt::UserRole + 1).toString());
          });
  connect(m_red, &eXRC::Service::Reddit::ready, this, &AppWindow::onReady);
  connect(m_red, &eXRC::Service::Reddit::grantError, this,
          &AppWindow::onGrantError);
  connect(m_red, &eXRC::Service::Reddit::grantExpired, this,
          &AppWindow::onRevoked);
  connect(m_red, &eXRC::Service::Reddit::revoked, this, &AppWindow::onRevoked);
  connect(m_red, &eXRC::Service::Reddit::revokeError, this,
          &AppWindow::onRevokeError);
  connect(m_authBtn, &QPushButton::clicked, this, &AppWindow::onAuthorize);
  connect(m_revokeBtn, &QPushButton::clicked, this, &AppWindow::onRevoke);
  connect(m_videoSelectBtn, &QPushButton::clicked, this,
          &AppWindow::onVideoFileSelectAndUpload);
  connect(m_red, &eXRC::Service::Reddit::subredditLinkFlairs, this,
          [this](const QString &subreddit, const QJsonArray &flairs) {
            if (subreddit == m_subredditLE->text()) {
              this->m_flairCompleter->setModel(new FlairListModel(flairs));
            }
          });
  connect(m_red, &eXRC::Service::Reddit::postRequirements, this,
          [this](const QString &subreddit, bool flairRequired) {
            if (subreddit == m_subredditLE->text() && flairRequired) {
              this->m_red->subredditLinkFlair(subreddit);
            }
          });
  connect(m_red, &eXRC::Service::Reddit::invalidSubreddit, this,
          [this](const QString &subreddit) {
            QMessageBox *msgBox = CreateMessageBox(
                QMessageBox::Warning, "Invalid Subreddit",
                "Subreddit " + m_subredditLE->text() + " doesn't exist!",
                QMessageBox::Ok, this);
            m_subredditLE->setText(QString());
            msgBox->exec();
            msgBox->deleteLater();
          });
  connect(m_subredditLE, &QLineEdit::editingFinished, this, [this]() {
    m_flairCompleter->setModel(new FlairListModel(QJsonArray()));

    if (m_subredditLE->text().isEmpty())
      return;

    this->m_red->postRequirement(m_subredditLE->text());
  });

  centralLayout->addWidget(m_authGB);
  centralLayout->addWidget(m_postGB);
  centralLayout->addStretch();
  centralWidget->setLayout(centralLayout);
  setCentralWidget(centralWidget);

#ifdef WIN32
  QTimer *themeChangeTimer = new QTimer(this);
  connect(themeChangeTimer, &QTimer::timeout, this, [this]() {
    bool darkMode = IsWindowsDarkMode();

    if (darkMode != m_darkMode) {
      m_darkMode = darkMode;
      SetDarkModeStatus(m_darkMode, HWND(winId()));
    }
  });
  themeChangeTimer->start(1000);
#endif // WIN32
}

void AppWindow::setupMenuBar() {
  QMenuBar *menuBar = new QMenuBar;
  QMenu *fileMenu = menuBar->addMenu("File");
  QMenu *helpMenu = menuBar->addMenu("Help");
  QAction *exitAct = fileMenu->addAction("Exit");
  connect(exitAct, &QAction::triggered, this,
          [this](bool checked) { QApplication::quit(); });
  QAction *aboutAct = helpMenu->addAction("About");
  QAction *aboutQtAct = helpMenu->addAction("About Qt");
  connect(aboutAct, &QAction::triggered, this, [this](bool checked) {
    QMessageBox *aboutMsgBox = CreateMessageBox(
        QMessageBox::NoIcon, "About",
        QString(APP_NAME) + " " + QString(APP_VERSION) +
            " - Reddit media poster via external media hosting platforms.<br />"
            "Copyright (C) 2022  eXhumer<br />"
            "<br />"
            "This program is free software: you can redistribute it and/or "
            "modify it under the terms of the GNU General Public License as "
            "published by the Free Software Foundation, version 3 of the "
            "License.<br />"
            "<br />"
            "This program is distributed in the hope that it will be useful, "
            "but WITHOUT ANY WARRANTY; without even the implied warranty of "
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
            "General Public License for more details.<br />"
            "<br />"
            "You should have received a copy of the GNU General Public License "
            "along with this program.  If not, see "
            "<a href='https://www.gnu.org/licenses/'>here</a>.<br />",
        QMessageBox::Ok, this);

    QIcon icon = aboutMsgBox->windowIcon();
    QSize size = icon.actualSize(QSize(64, 64));
    aboutMsgBox->setTextFormat(Qt::TextFormat::RichText);
    aboutMsgBox->setIconPixmap(icon.pixmap(size));
    aboutMsgBox->exec();
    aboutMsgBox->deleteLater();
  });
  connect(aboutQtAct, &QAction::triggered, this,
          [this](bool checked) { QMessageBox::aboutQt(this); });
  setMenuBar(menuBar);
}

void AppWindow::setupServices(const QString &redditClientId) {
  disableWidgets();
  QNetworkAccessManager *nam = new QNetworkAccessManager;
  m_media = new eXVHP::Service::MediaService(nam);

  QFile credentialFile(m_appDataDir.filePath("credential.json"));
  if (credentialFile.exists()) {
    credentialFile.open(QIODevice::ReadOnly);
    QJsonObject credentialData =
        QJsonDocument::fromJson(credentialFile.readAll()).object();
    QString accessToken = credentialData["access_token"].toString();
    QString refreshToken = credentialData["refresh_token"].toString();
    QDateTime expAt =
        QDateTime::fromSecsSinceEpoch(credentialData["expiry_at"].toInt());

    if (QDateTime::currentDateTime() < expAt || !refreshToken.isEmpty()) {
      m_red = new eXRC::Service::Reddit(redditClientId, nam, accessToken, expAt,
                                        refreshToken.isEmpty() ? QString()
                                                               : refreshToken);

      if (!refreshToken.isEmpty())
        m_permanentCB->setChecked(true);

      return;
    }
  }

  m_red = new eXRC::Service::Reddit(redditClientId, nam);
  onRevoked();
}

void AppWindow::setupWidgets() {
  m_permanentCB = new QCheckBox("Permanent");
  m_authGB = new QGroupBox("Reddit Authorization");
  m_postGB = new QGroupBox("Post Video");
  m_subredditLE = new QLineEdit;
  m_subredditLE->setAlignment(Qt::AlignHCenter);
  m_titleLE = new QLineEdit;
  m_titleLE->setAlignment(Qt::AlignHCenter);
  m_flairLE = new QLineEdit;
  m_flairLE->setAlignment(Qt::AlignHCenter);
  m_flairCompleter = new QCompleter(m_flairLE);
  m_flairCompleter->setFilterMode(Qt::MatchFlag::MatchContains);
  m_flairCompleter->setCompletionRole(Qt::UserRole);
  m_flairCompleter->setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
  m_flairLE->setCompleter(m_flairCompleter);
  m_uploadProgress = new QProgressBar;
  m_uploadProgress->setTextVisible(false);
  m_authBtn = new QPushButton("Authorize");
  m_revokeBtn = new QPushButton("Revoke");
  m_videoSelectBtn = new QPushButton("Select Video File and Upload");
  m_imgurRB = new QRadioButton("Imgur");
  m_jslRB = new QRadioButton("JustStreamLive");
  m_redRB = new QRadioButton("Reddit");
  m_sabRB = new QRadioButton("Streamable");
  m_sffRB = new QRadioButton("Streamff");
  m_sjaRB = new QRadioButton("Streamja");
  m_postNSFWCB = new QCheckBox("NSFW?");
  m_postSRCB = new QCheckBox("Send Replies?");
  m_postSpoilerCB = new QCheckBox("Spoiler?");
  m_authStateLE = new QLineEdit;
  m_authStateLE->setAlignment(Qt::AlignHCenter);
  m_authStateLE->setReadOnly(true);
}

void AppWindow::enableWidgets() {
  m_permanentCB->setEnabled(true);
  m_subredditLE->setEnabled(true);
  m_titleLE->setEnabled(true);
  m_flairLE->setEnabled(true);
  m_authBtn->setEnabled(true);
  m_revokeBtn->setEnabled(true);
  m_videoSelectBtn->setEnabled(true);
  m_imgurRB->setEnabled(true);
  m_jslRB->setEnabled(true);
  m_redRB->setEnabled(true);
  m_sabRB->setEnabled(true);
  m_sffRB->setEnabled(true);
  m_sjaRB->setEnabled(true);
  m_postNSFWCB->setEnabled(true);
  m_postSpoilerCB->setEnabled(true);
  m_postSRCB->setEnabled(true);
}

void AppWindow::disableWidgets() {
  m_permanentCB->setDisabled(true);
  m_subredditLE->setText(QString());
  m_subredditLE->setDisabled(true);
  m_titleLE->setText(QString());
  m_titleLE->setDisabled(true);
  m_flairLE->setText(QString());
  m_flairLE->setDisabled(true);
  m_authBtn->setDisabled(true);
  m_revokeBtn->setDisabled(true);
  m_videoSelectBtn->setDisabled(true);
  m_imgurRB->setDisabled(true);
  m_jslRB->setDisabled(true);
  m_redRB->setDisabled(true);
  m_sabRB->setDisabled(true);
  m_sffRB->setDisabled(true);
  m_sjaRB->setDisabled(true);
  m_postNSFWCB->setDisabled(true);
  m_postSpoilerCB->setDisabled(true);
  m_postSRCB->setDisabled(true);
}

void AppWindow::onAuthorize() {
  m_authBtn->setDisabled(true);
  m_permanentCB->setDisabled(true);
  m_red->grant(m_permanentCB->isChecked());
}

void AppWindow::onReady(const QJsonObject &identity) {
  QFile credentialFile(m_appDataDir.filePath("credential.json"));
  credentialFile.open(QIODevice::WriteOnly);
  QJsonObject credentialData;
  credentialData["access_token"] = m_red->token();
  credentialData["refresh_token"] = m_red->refreshToken();
  credentialData["expiry_at"] = m_red->expirationAt().toSecsSinceEpoch();
  credentialFile.write(
      QJsonDocument(credentialData).toJson(QJsonDocument::Compact));
  enableWidgets();
  m_authBtn->setDisabled(true);
  m_permanentCB->setDisabled(true);
  m_authStateLE->setText("Authorized as " + identity["name"].toString() + "!");
}

void AppWindow::onGrantError(const QString &error) {
  m_authBtn->setEnabled(true);

  if (error == "access_denied") {
    QMessageBox *msgBox = CreateMessageBox(
        QMessageBox::Warning, "User Denied Access",
        "Unable to continue unless user authorizes application!",
        QMessageBox::Ok, this);
    msgBox->exec();
    msgBox->deleteLater();
  }

  else {
    QMessageBox *msgBox = CreateMessageBox(
        QMessageBox::Warning, "Error occurred during authorization",
        "Error: " + error, QMessageBox::Ok, this);
    msgBox->exec();
    msgBox->deleteLater();
  }
}

void AppWindow::onRevoke() {
  m_revokeBtn->setDisabled(true);
  m_red->revoke();
}

void AppWindow::onRevoked() {
  QFile::remove(m_appDataDir.filePath("credential.json"));
  disableWidgets();
  m_authBtn->setEnabled(true);
  m_permanentCB->setEnabled(true);
  m_authStateLE->setText("Unauthorized!");
}

void AppWindow::onRevokeError(const QString &errorString) {
  m_revokeBtn->setEnabled(true);
  QMessageBox *msgBox = CreateMessageBox(QMessageBox::Warning,
                                         "Error occurred while revoking token",
                                         errorString, QMessageBox::Ok, this);
  msgBox->exec();
  msgBox->deleteLater();
}

void AppWindow::onVideoFileSelectAndUpload() {
  if (m_titleLE->text().isEmpty()) {
    QMessageBox *msgBox =
        CreateMessageBox(QMessageBox::Warning, "No post title specified",
                         "A post title must be specified for a Reddit post!",
                         QMessageBox::Ok, this);
    msgBox->exec();
    msgBox->deleteLater();
    return;
  }

  bool hostSelected = m_imgurRB->isChecked() || m_jslRB->isChecked() ||
                      m_redRB->isChecked() || m_sabRB->isChecked() ||
                      m_sffRB->isChecked() || m_sjaRB->isChecked();

  if (!hostSelected) {
    QMessageBox *msgBox = CreateMessageBox(
        QMessageBox::Warning, "No video host selected",
        "You need to select a video host before selecting the video file!",
        QMessageBox::Ok, this);
    msgBox->exec();
    msgBox->deleteLater();
    return;
  }

  QString filter;

  if (m_jslRB->isChecked())
    filter = "Supported Video Files (*.mkv *.mp4 *.webm)";

  else if (m_imgurRB->isChecked() || m_redRB->isChecked() ||
           m_sabRB->isChecked())
    filter = "Supported Video Files (*.mkv *.mp4)";

  else if (m_sffRB->isChecked() || m_sjaRB->isChecked())
    filter = "Supported Video Files (*.mp4)";

  QString videoFilePath = QFileDialog::getOpenFileName(
      this, "Select supported video file", QDir::homePath(), filter);

  if (videoFilePath.isNull())
    return;

  QFile *videoFile = new QFile(videoFilePath);
  QObject *videoCtx = new QObject;

  if (m_imgurRB->isChecked() || m_jslRB->isChecked() || m_sabRB->isChecked() ||
      m_sffRB->isChecked() || m_sjaRB->isChecked()) {
    connect(
        m_media, &eXVHP::Service::MediaService::mediaUploadProgress, videoCtx,
        [this, videoFile](QFile *vidFile, qint64 bytesSent, qint64 bytesTotal) {
          if (videoFile == vidFile) {
            m_uploadProgress->setValue(bytesSent);
            m_uploadProgress->setMaximum(bytesTotal);
          }
        });

    connect(
        m_media, &eXVHP::Service::MediaService::mediaUploaded, videoCtx,
        [this, videoCtx, videoFile](QFile *vidFile, const QString &videoId,
                                    const QString &videoLink) {
          if (videoFile == vidFile) {
            connect(m_red, &eXRC::Service::Reddit::postedUrl, videoCtx,
                    [this, videoCtx, videoLink](const QString &postUrl,
                                                const QString &redditUrl) {
                      if (postUrl == videoLink) {
                        QMessageBox *videoLinkMsgBox = new QMessageBox(
                            QMessageBox::NoIcon, "Video Posted Successfully!",
                            "Posted <a href=\"" + postUrl +
                                "\">Video</a> to <a href=\"" + redditUrl +
                                "\">Reddit</a>!",
                            QMessageBox::Ok, this);
                        QIcon icon = videoLinkMsgBox->windowIcon();
                        QSize size = icon.actualSize(QSize(64, 64));
                        videoLinkMsgBox->setIconPixmap(icon.pixmap(size));
                        videoLinkMsgBox->exec();
                        videoLinkMsgBox->deleteLater();
                        videoCtx->deleteLater();
                      }
                    });
            connect(m_red, &eXRC::Service::Reddit::postUrlError, videoCtx,
                    [this, videoCtx, videoLink](const QString &postUrl,
                                                const QString &error) {
                      if (postUrl == videoLink) {
                        QMessageBox *msgBox = CreateMessageBox(
                            QMessageBox::Warning, "Reddit Media Post Error!",
                            "Error while posting media link to Reddit!\n" +
                                error,
                            QMessageBox::Ok, this);
                        msgBox->exec();
                        msgBox->deleteLater();
                        videoCtx->deleteLater();
                      }
                    });

            m_red->postUrl(videoLink, m_titleLE->text(), m_subredditLE->text(),
                           m_flairLE->text(), m_postSRCB->isChecked(),
                           m_postNSFWCB->isChecked(),
                           m_postSpoilerCB->isChecked());
          }
        });

    connect(m_media, &eXVHP::Service::MediaService::mediaUploadError, videoCtx,
            [this, videoCtx, videoFile](QFile *vidFile, const QString &error) {
              if (videoFile == vidFile) {
                QMessageBox *msgBox =
                    CreateMessageBox(QMessageBox::Warning, "Media Upload Error",
                                     error, QMessageBox::Ok, this);
                msgBox->exec();
                msgBox->deleteLater();
                videoCtx->deleteLater();
              }
            });

    if (m_imgurRB->isChecked())
      m_media->uploadImgur(videoFile, m_titleLE->text());

    if (m_jslRB->isChecked())
      m_media->uploadJustStreamLive(videoFile);

    else if (m_sabRB->isChecked())
      m_media->uploadStreamable(videoFile, m_titleLE->text(), "us-east-1");

    else if (m_sffRB->isChecked())
      m_media->uploadStreamff(videoFile);

    else if (m_sjaRB->isChecked())
      m_media->uploadStreamja(videoFile);
  } else {
    connect(m_red, &eXRC::Service::Reddit::mediaUploadProgress, videoCtx,
            [this, videoFile](QFile *progressFile, qint64 bytesSent,
                              qint64 bytesTotal) {
              if (progressFile == videoFile) {
                m_uploadProgress->setValue(bytesSent);
                m_uploadProgress->setMaximum(bytesTotal);
              }
            });

    connect(
        m_red, &eXRC::Service::Reddit::mediaUploadError, videoCtx,
        [this, videoCtx, videoFile](QFile *errorFile, const QString &error) {
          if (errorFile == videoFile) {
            QMessageBox *msgBox =
                CreateMessageBox(QMessageBox::Warning,
                                 "Error occurred while uploading media file",
                                 error, QMessageBox::Ok, this);
            msgBox->exec();
            msgBox->deleteLater();
            videoCtx->deleteLater();
          }
        });

    connect(m_red, &eXRC::Service::Reddit::postMediaError, videoCtx,
            [this, videoCtx, videoFile](QFile *errorFile,
                                        QFile *videoThumbnailFile,
                                        const QString &error) {
              if (errorFile == videoFile) {
                QMessageBox *msgBox = CreateMessageBox(
                    QMessageBox::Warning,
                    "Error occurred while post uploaded media file to Reddit",
                    error, QMessageBox::Ok, this);
                msgBox->exec();
                msgBox->deleteLater();
                videoCtx->deleteLater();
              }
            });

    connect(m_red, &eXRC::Service::Reddit::postedMedia, videoCtx,
            [this, videoCtx, videoFile](QFile *mediaFile,
                                        QFile *videoThumbnailFile,
                                        const QString &postUrl) {
              if (mediaFile == videoFile) {
                QMessageBox *msgBox = CreateMessageBox(
                    QMessageBox::NoIcon, "Video Posted Successfully!",
                    "Posted to <a href=\"" + postUrl + "\">Reddit</a>!",
                    QMessageBox::Ok, this);

                QIcon icon = msgBox->windowIcon();
                QSize size = icon.actualSize(QSize(64, 64));
                msgBox->setIconPixmap(icon.pixmap(size));
                msgBox->exec();
                msgBox->deleteLater();
                videoCtx->deleteLater();
              }
            });

    m_red->postMedia(videoFile, nullptr, m_titleLE->text(),
                     m_subredditLE->text(), m_flairLE->text(),
                     m_postSRCB->isChecked(), m_postNSFWCB->isChecked(),
                     m_postSpoilerCB->isChecked());
  }
}
