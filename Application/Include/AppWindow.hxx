#ifndef APPWINDOW_HXX
#define APPWINDOW_HXX

#include "RedditService.hxx"
#include <QCheckBox>
#include <QGroupBox>
#include <QMainWindow>
#include <QPushButton>

class AppWindow : public QMainWindow {
  Q_OBJECT

private:
  RedditService *m_redditService;
  QCheckBox *m_permanentCB;
  QGroupBox *m_authGB;
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
