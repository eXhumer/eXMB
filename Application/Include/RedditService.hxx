#ifndef REDDITSERVICE_HXX
#define REDDITSERVICE_HXX

#include "RedditAuthorizationData.hxx"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>

class RedditService : public QObject {
  Q_OBJECT

private:
  RedditAuthorizationData *m_authData;
  QNetworkAccessManager *m_nam;
  QOAuthHttpServerReplyHandler *m_replyHandler;
  QOAuth2AuthorizationCodeFlow *m_authFlow;

public:
  RedditService(QString clientId, QString token, QDateTime expAt,
                QString refreshToken = QString(),
                QNetworkAccessManager *nam = nullptr,
                QObject *parent = nullptr);
  RedditService(QString clientId, QNetworkAccessManager *nam = nullptr,
                QObject *parent = nullptr);

private slots:
  void onGranted();
  void onTokenExpiry();

public slots:
  void grant(bool permanent);
  void revoke();

signals:
  void granted();
  void grantError(const QString &error, const QString &errorDescription,
                  const QUrl &uri);
  void grantExpired();
  void revoked();
  void revokeError(const QNetworkReply::NetworkError &error,
                   const QString &errorString);
};

#endif // REDDITSERVICE_HXX
