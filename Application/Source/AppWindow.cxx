#include "AppWindow.hxx"
#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>

AppWindow::AppWindow(QWidget *parent) : QMainWindow(parent) {
  this->setupCentralWidget();
  this->setupMenuBar();
}

void AppWindow::setupCentralWidget() {
  m_authBtn = new QPushButton("Authorize");
  m_revokeBtn = new QPushButton("Revoke");
  m_authGB = new QGroupBox("Reddit Authorization");
  m_permanentCB = new QCheckBox("Permanent");
  m_redditService = new RedditService("bnPnumDqM7YlDueRSzZCDw");
  QVBoxLayout *authLayout = new QVBoxLayout;
  QVBoxLayout *centralLayout = new QVBoxLayout;
  QWidget *centralWidget = new QWidget;

  m_authGB->setLayout(authLayout);
  authLayout->addWidget(m_permanentCB, 0, Qt::AlignHCenter);
  authLayout->addWidget(m_authBtn);
  authLayout->addWidget(m_revokeBtn);
  centralLayout->addWidget(m_authGB);
  centralLayout->addStretch();

  m_revokeBtn->setDisabled(true);

  connect(m_redditService, &RedditService::granted, this,
          &AppWindow::onGranted);
  connect(m_redditService, &RedditService::grantError, this,
          &AppWindow::onGrantError);
  connect(m_redditService, &RedditService::revoked, this,
          &AppWindow::onRevoked);
  connect(m_redditService, &RedditService::revokeError, this,
          &AppWindow::onRevokeError);
  connect(m_authBtn, &QPushButton::clicked, this, &AppWindow::onAuthorize);
  connect(m_revokeBtn, &QPushButton::clicked, this, &AppWindow::onRevoke);

  centralWidget->setLayout(centralLayout);
  setCentralWidget(centralWidget);
}

void AppWindow::setupMenuBar() {
  QMenuBar *menuBar = new QMenuBar;
  QMenu *fileMenu = menuBar->addMenu("File");
  QMenu *helpMenu = menuBar->addMenu("Help");
  QAction *exitAct = fileMenu->addAction("Exit");
  connect(exitAct, &QAction::triggered, this,
          [this](bool checked) { QApplication::quit(); });
  QAction *aboutQtAct = helpMenu->addAction("About Qt");
  connect(aboutQtAct, &QAction::triggered, this,
          [this](bool checked) { QMessageBox::aboutQt(this); });
  setMenuBar(menuBar);
}

void AppWindow::onAuthorize() {
  m_authBtn->setDisabled(true);
  m_permanentCB->setDisabled(true);
  m_redditService->grant(m_permanentCB->isChecked());
}

void AppWindow::onGranted() { m_revokeBtn->setEnabled(true); }

void AppWindow::onGrantError(const QString &error,
                             const QString &errorDescription, const QUrl &uri) {
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
  m_redditService->revoke();
}

void AppWindow::onRevoked() {
  m_authBtn->setEnabled(true);
  m_permanentCB->setEnabled(true);
}

void AppWindow::onRevokeError(const QNetworkReply::NetworkError &error,
                              const QString &errorString) {
  m_revokeBtn->setEnabled(true);
  QMessageBox::warning(this, "Error occurred while revoking token",
                       errorString);
}
