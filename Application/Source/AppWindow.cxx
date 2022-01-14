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
#include "eXRC/Reddit.hxx"
#include "eXVHP/JustStreamLive.hxx"
#include "eXVHP/Mixture.hxx"
#include "eXVHP/Streamable.hxx"
#include "eXVHP/Streamff.hxx"
#include "eXVHP/Streamja.hxx"
#include "eXVHP/Streamwo.hxx"
#include <QApplication>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
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
#endif // WIN32

AppWindow::AppWindow(const QString &redditClientId, QWidget *parent)
    : QMainWindow(parent), m_appDataDir(QStandardPaths::writableLocation(
                               QStandardPaths::AppDataLocation)) {
#ifdef WIN32
  m_darkMode = IsWindowsDarkMode();
  BOOL USE_DARK_MODE = m_darkMode;
  BOOL SET_CAPTION_COLOR_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
      HWND(winId()), DWMWINDOWATTRIBUTE::DWMWA_CAPTION_COLOR,
      USE_DARK_MODE ? &DARK_COLOR : &LIGHT_COLOR,
      sizeof(USE_DARK_MODE ? DARK_COLOR : LIGHT_COLOR)));
  BOOL SET_TEXT_COLOR_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
      HWND(winId()), DWMWINDOWATTRIBUTE::DWMWA_TEXT_COLOR,
      USE_DARK_MODE ? &DARK_TEXT_COLOR : &LIGHT_TEXT_COLOR,
      sizeof(USE_DARK_MODE ? DARK_TEXT_COLOR : LIGHT_TEXT_COLOR)));
  BOOL USE_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
      HWND(winId()), DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
      &USE_DARK_MODE, sizeof(USE_DARK_MODE)));
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
  postHostLayout->addWidget(m_jslRB);
  postHostLayout->addWidget(m_mixRB);
  postHostLayout->addWidget(m_sabRB);
  postHostLayout->addWidget(m_sffRB);
  postHostLayout->addWidget(m_sjaRB);
  postHostLayout->addWidget(m_swoRB);
  postLayout->addLayout(postHostLayout);
  postOptsLayout->addWidget(m_postNSFWCB);
  postOptsLayout->addWidget(m_postSRCB);
  postOptsLayout->addWidget(m_postSpoilerCB);
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
      BOOL USE_DARK_MODE = m_darkMode;
      BOOL SET_CAPTION_COLOR_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
          HWND(winId()), DWMWINDOWATTRIBUTE::DWMWA_CAPTION_COLOR,
          USE_DARK_MODE ? &DARK_COLOR : &LIGHT_COLOR,
          sizeof(USE_DARK_MODE ? DARK_COLOR : LIGHT_COLOR)));
      BOOL SET_TEXT_COLOR_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
          HWND(winId()), DWMWINDOWATTRIBUTE::DWMWA_TEXT_COLOR,
          USE_DARK_MODE ? &DARK_TEXT_COLOR : &LIGHT_TEXT_COLOR,
          sizeof(USE_DARK_MODE ? DARK_TEXT_COLOR : LIGHT_TEXT_COLOR)));
      BOOL USE_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
          HWND(winId()), DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
          &USE_DARK_MODE, sizeof(USE_DARK_MODE)));
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
    QMessageBox::about(this, "About",
                       QString(APP_NAME) + " v" + QString(APP_VERSION) +
                           "\n\nReddit media poster via external media hosting "
                           "platforms.\n\nCopyright (C) 2022 - eXhumer");
  });
  connect(aboutQtAct, &QAction::triggered, this,
          [this](bool checked) { QMessageBox::aboutQt(this); });
  setMenuBar(menuBar);
}

void AppWindow::setupServices(const QString &redditClientId) {
  disableWidgets();
  QNetworkAccessManager *nam = new QNetworkAccessManager;
  m_jsl = new eXVHP::Service::JustStreamLive(nam);
  m_mix = new eXVHP::Service::Mixture(nam);
  m_sab = new eXVHP::Service::Streamable(nam);
  m_sff = new eXVHP::Service::Streamff(nam);
  m_sja = new eXVHP::Service::Streamja(nam);
  m_swo = new eXVHP::Service::Streamwo(nam);

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
  m_uploadProgress = new QProgressBar;
  m_uploadProgress->setTextVisible(false);
  m_authBtn = new QPushButton("Authorize");
  m_revokeBtn = new QPushButton("Revoke");
  m_videoSelectBtn = new QPushButton("Select Video File and Upload");
  m_jslRB = new QRadioButton("JustStreamLive");
  m_mixRB = new QRadioButton("Mixture");
  m_sabRB = new QRadioButton("Streamable");
  m_sffRB = new QRadioButton("Streamff");
  m_sjaRB = new QRadioButton("Streamja");
  m_swoRB = new QRadioButton("Streamwo");
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
  m_jslRB->setEnabled(true);
  m_mixRB->setEnabled(true);
  m_sabRB->setEnabled(true);
  m_sffRB->setEnabled(true);
  m_sjaRB->setEnabled(true);
  m_swoRB->setEnabled(true);
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
  m_jslRB->setDisabled(true);
  m_mixRB->setDisabled(true);
  m_sabRB->setDisabled(true);
  m_sffRB->setDisabled(true);
  m_sjaRB->setDisabled(true);
  m_swoRB->setDisabled(true);
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

  if (error == "access_denied")
    QMessageBox::warning(
        this, "User Denied Access",
        "Unable to continue unless user authorizes application!");

  else
    QMessageBox::warning(this, "Error occurred during authorization",
                         "Error: " + error);
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
  QMessageBox::warning(this, "Error occurred while revoking token",
                       errorString);
}

void AppWindow::onVideoFileSelectAndUpload() {
  if (m_titleLE->text().isEmpty()) {
    QMessageBox::warning(this, "No post title specified",
                         "A post title must be specified for a Reddit post!");
    return;
  }

  bool hostSelected = m_jslRB->isChecked() || m_mixRB->isChecked() ||
                      m_sabRB->isChecked() || m_sffRB->isChecked() ||
                      m_sjaRB->isChecked() || m_swoRB->isChecked();

  if (!hostSelected) {
    QMessageBox::warning(
        this, "No video host selected",
        "You need to select a video host before selecting the video file!");
    return;
  }

  QString filter;

  if (m_jslRB->isChecked())
    filter = "Supported Video Files (*.mkv *.mp4 *.webm)";

  else if (m_sabRB->isChecked())
    filter = "Supported Video Files (*.mkv *.mp4)";

  else if (m_mixRB->isChecked() || m_sffRB->isChecked() ||
           m_sjaRB->isChecked() || m_swoRB->isChecked())
    filter = "Supported Video Files (*.mp4)";

  QString videoFilePath = QFileDialog::getOpenFileName(
      this, "Select supported video file", QDir::homePath(), filter);

  if (videoFilePath.isNull())
    return;

  QFile *videoFile = new QFile(videoFilePath);

  if (m_jslRB->isChecked()) {
    QProgressBar *uploadProgress = m_uploadProgress;
    connect(
        m_jsl, &eXVHP::Service::JustStreamLive::videoUploadProgress, this,
        [uploadProgress, videoFile](QFile *vidFile, qint64 bytesSent,
                                    qint64 bytesTotal) {
          if (videoFile == vidFile) {
            uploadProgress->setValue(bytesSent);
            uploadProgress->setMaximum(bytesTotal);
          }
        },
        Qt::UniqueConnection);

    connect(
        m_jsl, &eXVHP::Service::JustStreamLive::videoUploaded, this,
        [this, videoFile](QFile *vidFile, const QString &linkId,
                          const QString &videoLink) {
          if (videoFile == vidFile) {
            connect(
                m_red, &eXRC::Service::Reddit::postedUrl, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &redditUrl) {
                  if (postUrl == videoLink) {
                    QMessageBox::information(this, "Video Posted Successfully!",
                                             "Posted <a href=\"" + postUrl +
                                                 "\">Video</a> to <a href=\"" +
                                                 redditUrl + "\">Reddit</a>!");
                  }
                },
                Qt::UniqueConnection);
            connect(
                m_red, &eXRC::Service::Reddit::postUrlError, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &error) {
                  if (postUrl == videoLink) {
                    QMessageBox::warning(
                        this, "Video Posted Error!",
                        "Error while posting video link to Reddit!\n" + error);
                  }
                },
                Qt::UniqueConnection);

            m_red->postUrl(videoLink, m_titleLE->text(), m_subredditLE->text(),
                           m_flairLE->text(), m_postSRCB->isChecked(),
                           m_postNSFWCB->isChecked(),
                           m_postSpoilerCB->isChecked());
          }
        },
        Qt::UniqueConnection);

    connect(
        m_jsl, &eXVHP::Service::JustStreamLive::videoUploadError, this,
        [this, videoFile](QFile *vidFile, const QString &error) {
          if (videoFile == vidFile) {
            QMessageBox::warning(this, "JustStreamLive Upload Error", error);
          }
        },
        Qt::UniqueConnection);

    m_jsl->uploadVideo(videoFile);
  } else if (m_mixRB->isChecked()) {
    QProgressBar *uploadProgress = m_uploadProgress;
    connect(
        m_mix, &eXVHP::Service::Mixture::videoUploadProgress, this,
        [uploadProgress, videoFile](QFile *vidFile, qint64 bytesSent,
                                    qint64 bytesTotal) {
          if (videoFile == vidFile) {
            uploadProgress->setValue(bytesSent);
            uploadProgress->setMaximum(bytesTotal);
          }
        },
        Qt::UniqueConnection);

    connect(
        m_mix, &eXVHP::Service::Mixture::videoUploaded, this,
        [this, videoFile](QFile *vidFile, const QString &linkId,
                          const QString &videoLink) {
          if (videoFile == vidFile) {
            connect(
                m_red, &eXRC::Service::Reddit::postedUrl, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &redditUrl) {
                  if (postUrl == videoLink) {
                    QMessageBox::information(this, "Video Posted Successfully!",
                                             "Posted <a href=\"" + postUrl +
                                                 "\">Video</a> to <a href=\"" +
                                                 redditUrl + "\">Reddit</a>!");
                  }
                },
                Qt::UniqueConnection);
            connect(
                m_red, &eXRC::Service::Reddit::postUrlError, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &error) {
                  if (postUrl == videoLink) {
                    QMessageBox::warning(
                        this, "Video Posted Error!",
                        "Error while posting video link to Reddit!\n" + error);
                  }
                },
                Qt::UniqueConnection);

            m_red->postUrl(videoLink, m_titleLE->text(), m_subredditLE->text(),
                           m_flairLE->text(), m_postSRCB->isChecked(),
                           m_postNSFWCB->isChecked(),
                           m_postSpoilerCB->isChecked());
          }
        },
        Qt::UniqueConnection);

    connect(
        m_mix, &eXVHP::Service::Mixture::videoUploadError, this,
        [this, videoFile](QFile *vidFile, const QString &error) {
          if (videoFile == vidFile) {
            QMessageBox::warning(this, "Mixture Video Upload Error", error);
          }
        },
        Qt::UniqueConnection);

    m_mix->uploadVideo(videoFile);
  } else if (m_sabRB->isChecked()) {
    QProgressBar *uploadProgress = m_uploadProgress;
    connect(
        m_sab, &eXVHP::Service::Streamable::videoUploadProgress, this,
        [uploadProgress, videoFile](QFile *vidFile, qint64 bytesSent,
                                    qint64 bytesTotal) {
          if (videoFile == vidFile) {
            uploadProgress->setValue(bytesSent);
            uploadProgress->setMaximum(bytesTotal);
          }
        },
        Qt::UniqueConnection);

    connect(
        m_sab, &eXVHP::Service::Streamable::videoUploaded, this,
        [this, videoFile](QFile *vidFile, const QString &linkId,
                          const QString &videoLink) {
          if (videoFile == vidFile) {
            connect(
                m_red, &eXRC::Service::Reddit::postedUrl, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &redditUrl) {
                  if (postUrl == videoLink) {
                    QMessageBox::information(this, "Video Posted Successfully!",
                                             "Posted <a href=\"" + postUrl +
                                                 "\">Video</a> to <a href=\"" +
                                                 redditUrl + "\">Reddit</a>!");
                  }
                },
                Qt::UniqueConnection);
            connect(
                m_red, &eXRC::Service::Reddit::postUrlError, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &error) {
                  if (postUrl == videoLink) {
                    QMessageBox::warning(
                        this, "Video Posted Error!",
                        "Error while posting video link to Reddit!\n" + error);
                  }
                },
                Qt::UniqueConnection);

            m_red->postUrl(videoLink, m_titleLE->text(), m_subredditLE->text(),
                           m_flairLE->text(), m_postSRCB->isChecked(),
                           m_postNSFWCB->isChecked(),
                           m_postSpoilerCB->isChecked());
          }
        },
        Qt::UniqueConnection);

    connect(
        m_sab, &eXVHP::Service::Streamable::videoUploadError, this,
        [this, videoFile](QFile *vidFile, const QString &error) {
          if (videoFile == vidFile) {
            QMessageBox::warning(this, "Streamable Video Upload Error", error);
          }
        },
        Qt::UniqueConnection);

    m_sab->uploadVideo(videoFile, m_titleLE->text(), "us-east-1");
  } else if (m_sffRB->isChecked()) {
    QProgressBar *uploadProgress = m_uploadProgress;
    connect(
        m_sff, &eXVHP::Service::Streamff::videoUploadProgress, this,
        [uploadProgress, videoFile](QFile *vidFile, qint64 bytesSent,
                                    qint64 bytesTotal) {
          if (videoFile == vidFile) {
            uploadProgress->setValue(bytesSent);
            uploadProgress->setMaximum(bytesTotal);
          }
        },
        Qt::UniqueConnection);

    connect(
        m_sff, &eXVHP::Service::Streamff::videoUploaded, this,
        [this, videoFile](QFile *vidFile, const QString &linkId,
                          const QString &videoLink) {
          if (videoFile == vidFile) {
            connect(
                m_red, &eXRC::Service::Reddit::postedUrl, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &redditUrl) {
                  if (postUrl == videoLink) {
                    QMessageBox::information(this, "Video Posted Successfully!",
                                             "Posted <a href=\"" + postUrl +
                                                 "\">Video</a> to <a href=\"" +
                                                 redditUrl + "\">Reddit</a>!");
                  }
                },
                Qt::UniqueConnection);
            connect(
                m_red, &eXRC::Service::Reddit::postUrlError, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &error) {
                  if (postUrl == videoLink) {
                    QMessageBox::warning(
                        this, "Video Posted Error!",
                        "Error while posting video link to Reddit!\n" + error);
                  }
                },
                Qt::UniqueConnection);

            m_red->postUrl(videoLink, m_titleLE->text(), m_subredditLE->text(),
                           m_flairLE->text(), m_postSRCB->isChecked(),
                           m_postNSFWCB->isChecked(),
                           m_postSpoilerCB->isChecked());
          }
        },
        Qt::UniqueConnection);

    connect(
        m_sff, &eXVHP::Service::Streamff::videoUploadError, this,
        [this, videoFile](QFile *vidFile, const QString &error) {
          if (videoFile == vidFile) {
            QMessageBox::warning(this, "Streamff Video Upload Error", error);
          }
        },
        Qt::UniqueConnection);

    m_sff->uploadVideo(videoFile);
  } else if (m_sjaRB->isChecked()) {
    QProgressBar *uploadProgress = m_uploadProgress;
    connect(
        m_sja, &eXVHP::Service::Streamja::videoUploadProgress, this,
        [uploadProgress, videoFile](QFile *vidFile, qint64 bytesSent,
                                    qint64 bytesTotal) {
          if (videoFile == vidFile) {
            uploadProgress->setValue(bytesSent);
            uploadProgress->setMaximum(bytesTotal);
          }
        },
        Qt::UniqueConnection);

    connect(
        m_sja, &eXVHP::Service::Streamja::videoUploaded, this,
        [this, videoFile](QFile *vidFile, const QString &linkId,
                          const QString &videoLink,
                          const QString &videoEmbedLink) {
          if (videoFile == vidFile) {
            connect(
                m_red, &eXRC::Service::Reddit::postedUrl, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &redditUrl) {
                  if (postUrl == videoLink) {
                    QMessageBox::information(this, "Video Posted Successfully!",
                                             "Posted <a href=\"" + postUrl +
                                                 "\">Video</a> to <a href=\"" +
                                                 redditUrl + "\">Reddit</a>!");
                  }
                },
                Qt::UniqueConnection);
            connect(
                m_red, &eXRC::Service::Reddit::postUrlError, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &error) {
                  if (postUrl == videoLink) {
                    QMessageBox::warning(
                        this, "Video Posted Error!",
                        "Error while posting video link to Reddit!\n" + error);
                  }
                },
                Qt::UniqueConnection);

            m_red->postUrl(videoLink, m_titleLE->text(), m_subredditLE->text(),
                           m_flairLE->text(), m_postSRCB->isChecked(),
                           m_postNSFWCB->isChecked(),
                           m_postSpoilerCB->isChecked());
          }
        },
        Qt::UniqueConnection);

    connect(
        m_sja, &eXVHP::Service::Streamja::videoUploadError, this,
        [this, videoFile](QFile *vidFile, const QString &error) {
          if (videoFile == vidFile) {
            QMessageBox::warning(this, "Streamja Video Upload Error", error);
          }
        },
        Qt::UniqueConnection);

    m_sja->uploadVideo(videoFile);
  } else if (m_swoRB->isChecked()) {
    QProgressBar *uploadProgress = m_uploadProgress;
    connect(
        m_swo, &eXVHP::Service::Streamwo::videoUploadProgress, this,
        [uploadProgress, videoFile](QFile *vidFile, qint64 bytesSent,
                                    qint64 bytesTotal) {
          if (videoFile == vidFile) {
            uploadProgress->setValue(bytesSent);
            uploadProgress->setMaximum(bytesTotal);
          }
        },
        Qt::UniqueConnection);

    connect(
        m_swo, &eXVHP::Service::Streamwo::videoUploaded, this,
        [this, videoFile](QFile *vidFile, const QString &linkId,
                          const QString &videoLink) {
          if (videoFile == vidFile) {
            connect(
                m_red, &eXRC::Service::Reddit::postedUrl, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &redditUrl) {
                  if (postUrl == videoLink) {
                    QMessageBox::information(this, "Video Posted Successfully!",
                                             "Posted <a href=\"" + postUrl +
                                                 "\">Video</a> to <a href=\"" +
                                                 redditUrl + "\">Reddit</a>!");
                  }
                },
                Qt::UniqueConnection);
            connect(
                m_red, &eXRC::Service::Reddit::postUrlError, this,
                [this, videoLink](const QString &postUrl,
                                  const QString &error) {
                  if (postUrl == videoLink) {
                    QMessageBox::warning(
                        this, "Video Posted Error!",
                        "Error while posting video link to Reddit!\n" + error);
                  }
                },
                Qt::UniqueConnection);

            m_red->postUrl(videoLink, m_titleLE->text(), m_subredditLE->text(),
                           m_flairLE->text(), m_postSRCB->isChecked(),
                           m_postNSFWCB->isChecked(),
                           m_postSpoilerCB->isChecked());
          }
        },
        Qt::UniqueConnection);

    connect(
        m_swo, &eXVHP::Service::Streamwo::videoUploadError, this,
        [this, videoFile](QFile *vidFile, const QString &error) {
          if (videoFile == vidFile) {
            QMessageBox::warning(this, "Streamwo Video Upload Error", error);
          }
        },
        Qt::UniqueConnection);

    m_swo->uploadVideo(videoFile);
  }
}
