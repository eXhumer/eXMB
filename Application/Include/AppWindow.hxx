#ifndef APPWINDOW_HXX
#define APPWINDOW_HXX

namespace eXRC::Service {
class Reddit;
} // namespace eXRC::Service

namespace eXVHP::Service {
class JustStreamLive;
class Mixture;
class Streamable;
class Streamff;
class Streamja;
class Streamwo;
} // namespace eXVHP::Service

#include <QCheckBox>
#include <QDir>
#include <QGroupBox>
#include <QLineEdit>
#include <QMainWindow>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>

class AppWindow : public QMainWindow {
  Q_OBJECT

private:
  QDir m_appDataDir;
  eXRC::Service::Reddit *m_red;
  eXVHP::Service::JustStreamLive *m_jsl;
  eXVHP::Service::Mixture *m_mix;
  eXVHP::Service::Streamable *m_sab;
  eXVHP::Service::Streamff *m_sff;
  eXVHP::Service::Streamja *m_sja;
  eXVHP::Service::Streamwo *m_swo;
  QCheckBox *m_permanentCB;
  QGroupBox *m_authGB, *m_postGB;
  QLineEdit *m_subredditLE, *m_titleLE, *m_flairLE;
  QProgressBar *m_uploadProgress;
  QPushButton *m_authBtn, *m_revokeBtn, *m_videoSelectBtn;
  QRadioButton *m_jslRB, *m_mixRB, *m_sabRB, *m_sffRB, *m_sjaRB, *m_swoRB;
  void setupCentralWidget();
  void setupMenuBar();
  void setupServices();
  void setupWidgets();

public:
  AppWindow(QWidget *parent = nullptr);

private slots:
  void onAuthorize();
  void onReady(const QJsonObject &identity);
  void onGrantError(const QString &error);
  void onRevoke();
  void onRevoked();
  void onRevokeError(const QString &error);
  void onVideoFileSelectAndUpload();
};

#endif // APPWINDOW_HXX
