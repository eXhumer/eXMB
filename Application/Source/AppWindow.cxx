#include "AppWindow.hxx"
#include <QApplication>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>

AppWindow::AppWindow(QWidget *parent) : QMainWindow(parent) {
  this->setupCentralWidget();
  this->setupMenuBar();
}

void AppWindow::setupCentralWidget() {
  m_authBtn = new QPushButton("Authorize");
  m_authGB = new QGroupBox("Reddit Authorization");
  m_expAtL = new QLabel("Expiration At:");
  m_expAtLE = new QLineEdit;
  m_permanentCB = new QCheckBox("Permanent");
  m_redditAuth = new RedditAuthorization;
  m_refreshTokenL = new QLabel("Refresh Token:");
  m_refreshTokenLE = new QLineEdit;
  m_revokeBtn = new QPushButton("Revoke");
  m_tokenL = new QLabel("Token:");
  m_tokenLE = new QLineEdit;
  QGridLayout *authDataLayout = new QGridLayout;
  QHBoxLayout *expirationAtLineLayout = new QHBoxLayout;
  QVBoxLayout *authLayout = new QVBoxLayout;
  QVBoxLayout *centralLayout = new QVBoxLayout;
  QWidget *centralWidget = new QWidget;

  authDataLayout->addWidget(m_tokenL, 0, 0, Qt::AlignRight);
  authDataLayout->addWidget(m_tokenLE, 0, 1);
  authDataLayout->addWidget(m_refreshTokenL, 1, 0, Qt::AlignRight);
  authDataLayout->addWidget(m_refreshTokenLE, 1, 1);
  authDataLayout->addWidget(m_expAtL, 2, 0, Qt::AlignRight);
  authDataLayout->addWidget(m_expAtLE, 2, 1);
  m_authGB->setLayout(authLayout);
  authLayout->addLayout(authDataLayout);
  authLayout->addWidget(m_permanentCB, 0, Qt::AlignHCenter);
  authLayout->addWidget(m_authBtn);
  authLayout->addWidget(m_revokeBtn);
  centralLayout->addWidget(m_authGB);
  centralLayout->addStretch();

  m_expAtLE->setReadOnly(true);
  m_refreshTokenLE->setReadOnly(true);
  m_revokeBtn->setDisabled(true);
  m_tokenLE->setReadOnly(true);

  connect(this->m_redditAuth, &RedditAuthorization::granted, this,
          &AppWindow::onGranted);
  connect(this->m_redditAuth, &RedditAuthorization::grantError, this,
          &AppWindow::onGrantError);
  connect(this->m_redditAuth, &RedditAuthorization::revoked, this,
          &AppWindow::onRevoked);
  connect(this->m_redditAuth, &RedditAuthorization::revokeError, this,
          &AppWindow::onRevokeError);
  connect(this->m_authBtn, &QPushButton::clicked, this,
          &AppWindow::onAuthorize);
  connect(this->m_revokeBtn, &QPushButton::clicked, this, &AppWindow::onRevoke);

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
  m_redditAuth->grant(m_permanentCB->isChecked());
}

void AppWindow::onGranted() {
  m_tokenLE->setText(m_redditAuth->data()->token());
  m_refreshTokenLE->setText(m_redditAuth->data()->refreshToken().isEmpty()
                                ? "No Refresh Token Available!"
                                : m_redditAuth->data()->refreshToken());
  m_expAtLE->setText(m_redditAuth->data()->expirationAt().toString(
      "hh:mm:ss AP t, d MMMM yyyy"));
  m_revokeBtn->setEnabled(true);
}

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
  m_redditAuth->revoke();
}

void AppWindow::onRevoked() {
  m_authBtn->setEnabled(true);
  m_tokenLE->setText(QString());
  m_refreshTokenLE->setText(QString());
  m_expAtLE->setText(QString());
  m_permanentCB->setEnabled(true);
}

void AppWindow::onRevokeError(const QNetworkReply::NetworkError &error,
                              const QString &errorString) {
  m_revokeBtn->setEnabled(true);
  QMessageBox::warning(this, "Error occurred while revoking token",
                       errorString);
}
