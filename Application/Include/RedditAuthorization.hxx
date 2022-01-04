#ifndef REDDITAUTHORIZATION_HXX
#define REDDITAUTHORIZATION_HXX

#include "RedditAuthorizationData.hxx"
#include <QNetworkReply>
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>

class RedditAuthorization : public QObject {
  Q_OBJECT

private:
  RedditAuthorizationData *m_data;
  QOAuthHttpServerReplyHandler *m_replyHandler;
  QOAuth2AuthorizationCodeFlow *m_authFlow;

private slots:
  void onGranted();

public:
  RedditAuthorization(QObject *parent = nullptr);
  QString clientIdentifier() const;
  RedditAuthorizationData *data() const;

public slots:
  void grant(bool permanent);
  void revoke();
  void setNetworkAccessManager(QNetworkAccessManager *nam);

signals:
  void granted();
  void grantError(const QString &error, const QString &errorDescription,
                  const QUrl &uri);
  void revoked();
  void revokeError(const QNetworkReply::NetworkError &error,
                   const QString &errorString);
};

#endif // REDDITAUTHORIZATION_HXX
