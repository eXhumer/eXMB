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
#include "eXVHP/Service.hxx"
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

QMessageBox *CreateMessageBox(QMessageBox::Icon icon, const QString &title,
                              const QString &text,
                              QMessageBox::StandardButtons buttons =
                                  QMessageBox::StandardButton::NoButton,
                              QWidget *parent = nullptr) {
  QMessageBox *msgBox = new QMessageBox(icon, title, text, buttons, parent);
#ifdef WIN32
  BOOL IsDarkTheme = IsWindowsDarkMode();
  SetDarkModeStatus(IsDarkTheme, HWND(msgBox->winId()));

  QTimer *themeChangeTimer = new QTimer(msgBox);
  bool darkMode = IsDarkTheme;
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

AppWindow::AppWindow(QWidget *parent) : QMainWindow(parent) {
#ifdef WIN32
  m_darkMode = IsWindowsDarkMode();
  SetDarkModeStatus(m_darkMode, HWND(winId()));
#endif // WIN32
  QFile appQssFile(":/AppWindowDark.qss");
  appQssFile.open(QIODevice::ReadOnly);
  setStyleSheet(QString(appQssFile.readAll()));

  setupWidgets();
  setupServices();
  setupCentralWidget();
  setupMenuBar();
}

void AppWindow::setupCentralWidget() {
  QVBoxLayout *uploadLayout = new QVBoxLayout;
  QVBoxLayout *centralLayout = new QVBoxLayout;
  QWidget *centralWidget = new QWidget;

  uploadLayout->addWidget(m_titleLE);
  uploadLayout->addWidget(m_videoSelectBtn);
  uploadLayout->addWidget(m_uploadProgress);
  m_uploadGB->setLayout(uploadLayout);
  connect(m_videoSelectBtn, &QPushButton::clicked, this,
          &AppWindow::onVideoFileSelectAndUpload);

  centralLayout->addWidget(m_uploadGB);
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
        QString(APP_NAME) + " v" + QString(APP_VERSION) +
            "\n\nStreamable Video Uploader.\n\nCopyright (C) 2022 - eXhumer",
        QMessageBox::Ok, this);

    QIcon icon = aboutMsgBox->windowIcon();
    QSize size = icon.actualSize(QSize(64, 64));
    aboutMsgBox->setIconPixmap(icon.pixmap(size));
    aboutMsgBox->exec();
    aboutMsgBox->deleteLater();
  });
  connect(aboutQtAct, &QAction::triggered, this,
          [this](bool checked) { QMessageBox::aboutQt(this); });
  setMenuBar(menuBar);
}

void AppWindow::setupServices() {
  QNetworkAccessManager *nam = new QNetworkAccessManager;
  m_media = new eXVHP::Service::MediaService(nam);
}

void AppWindow::setupWidgets() {
  m_uploadGB = new QGroupBox("Upload Video");
  m_titleLE = new QLineEdit;
  m_titleLE->setAlignment(Qt::AlignHCenter);
  m_uploadProgress = new QProgressBar;
  m_uploadProgress->setTextVisible(false);
  m_videoSelectBtn = new QPushButton("Select Video File and Upload");
}

void AppWindow::onVideoFileSelectAndUpload() {
  QString videoFilePath = QFileDialog::getOpenFileName(
      this, "Select supported video file", QDir::homePath(),
      "Supported Video Files (*.mkv *.mp4)");

  if (videoFilePath.isNull())
    return;

  QFile *videoFile = new QFile(videoFilePath);
  QObject *videoCtx = new QObject;

  connect(
      m_media, &eXVHP::Service::MediaService::mediaUploadProgress, videoCtx,
      [this, videoFile](QFile *vidFile, qint64 bytesSent, qint64 bytesTotal) {
        if (videoFile == vidFile) {
          m_uploadProgress->setValue(bytesSent);
          m_uploadProgress->setMaximum(bytesTotal);
        }
      },
      Qt::UniqueConnection);

  connect(
      m_media, &eXVHP::Service::MediaService::mediaUploaded, videoCtx,
      [this, videoCtx, videoFile](QFile *vidFile, const QString &videoId,
                                  const QString &videoLink) {
        if (videoFile == vidFile) {
          QMessageBox *videoLinkMsgBox = CreateMessageBox(
              QMessageBox::NoIcon, "Video Posted Successfully!",
              "<a href=\"" + videoLink + "\">Video Link</a>", QMessageBox::Ok,
              this);

          QIcon icon = videoLinkMsgBox->windowIcon();
          QSize size = icon.actualSize(QSize(64, 64));
          videoLinkMsgBox->setIconPixmap(icon.pixmap(size));
          videoLinkMsgBox->exec();
          videoLinkMsgBox->deleteLater();
          videoCtx->deleteLater();
        }
      },
      Qt::UniqueConnection);

  connect(
      m_media, &eXVHP::Service::MediaService::mediaUploadError, videoCtx,
      [this, videoCtx, videoFile](QFile *vidFile, const QString &error) {
        if (videoFile == vidFile) {
          QMessageBox *videoLinkMsgBox =
              CreateMessageBox(QMessageBox::Warning, "Media Upload Error",
                               error, QMessageBox::Ok, this);
          videoLinkMsgBox->exec();
          videoLinkMsgBox->deleteLater();
          videoCtx->deleteLater();
        }
      },
      Qt::UniqueConnection);

  m_media->uploadStreamable(videoFile, m_titleLE->text(), "us-east-1");
}
