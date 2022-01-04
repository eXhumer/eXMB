#ifndef APPWINDOW_HXX
#define APPWINDOW_HXX

#include "RedditAuthorization.hxx"
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>

class AppWindow : public QMainWindow {
  Q_OBJECT

private:
  RedditAuthorization *m_redditAuth;
  QCheckBox *m_permanentCB;
  QGroupBox *m_authGB;
  QLabel *m_refreshTokenL, *m_tokenL, *m_expAtL;
  QLineEdit *m_refreshTokenLE, *m_tokenLE, *m_expAtLE;
  QPushButton *m_authBtn, *m_revokeBtn;
  void setupCentralWidget();
  void setupMenuBar();

public:
  AppWindow(QWidget *parent = nullptr);

private slots:
  void onAuthorize();
  void onGranted();
  void onGrantError(const QString &error, const QString &errorDescription,
                    const QUrl &uri);
  void onRevoke();
  void onRevoked();
  void onRevokeError(const QNetworkReply::NetworkError &error,
                     const QString &errorString);
};

#endif // APPWINDOW_HXX
