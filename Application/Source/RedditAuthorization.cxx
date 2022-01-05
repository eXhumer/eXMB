#include "RedditAuthorization.hxx"
#include "AppConfig.hxx"
#include <QAuthenticator>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QUrlQuery>

void RedditAuthorization::onGranted() {
  this->data()->setExpirationAt(m_authFlow->expirationAt());
  this->data()->setRefreshToken(m_authFlow->refreshToken());
  this->data()->setToken(m_authFlow->token());
  emit this->granted();
}

RedditAuthorization::RedditAuthorization(QObject *parent) : QObject(parent) {
  m_data = new RedditAuthorizationData;
  m_authFlow = new QOAuth2AuthorizationCodeFlow;
  m_authFlow->setAccessTokenUrl(
      QUrl("https://www.reddit.com/api/v1/access_token"));
  m_authFlow->setAuthorizationUrl(
      QUrl("https://www.reddit.com/api/v1/authorize"));
  m_authFlow->setClientIdentifier("bnPnumDqM7YlDueRSzZCDw");
  m_authFlow->setScope("identity");
  m_authFlow->setUserAgent(APP_NAME "/" APP_VERSION);
  m_replyHandler = new QOAuthHttpServerReplyHandler(65010);
  m_replyHandler->setCallbackPath("auth_callback");
  m_authFlow->setReplyHandler(m_replyHandler);

  connect(m_authFlow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
          &QDesktopServices::openUrl);
  connect(m_authFlow, &QOAuth2AuthorizationCodeFlow::granted, this,
          &RedditAuthorization::onGranted);
  connect(m_authFlow, &QOAuth2AuthorizationCodeFlow::error, this,
          &RedditAuthorization::grantError);
}

QString RedditAuthorization::clientIdentifier() const {
  return m_authFlow->clientIdentifier();
}

RedditAuthorizationData *RedditAuthorization::data() const { return m_data; }

void RedditAuthorization::grant(bool permanent) {
  m_authFlow->setModifyParametersFunction(
      [this, permanent](QAbstractOAuth::Stage stage,
                        QMultiMap<QString, QVariant> *parameters) {
        if (stage == QAbstractOAuth::Stage::RequestingAuthorization &&
            permanent)
          parameters->insert("duration", "permanent");
      });

  m_authFlow->grant();
}

void RedditAuthorization::revoke() {
  if (!m_authFlow->token().isEmpty()) {
    QUrlQuery revokeData;
    revokeData.addQueryItem("token", this->data()->token());
    revokeData.addQueryItem("token_token_hint", "access_token");

    QNetworkReply *res = m_authFlow->networkAccessManager()->post(
        QNetworkRequest(QUrl("https://www.reddit.com/api/v1/revoke_token")),
        revokeData.toString(QUrl::FullyEncoded).toUtf8());

    connect(m_authFlow->networkAccessManager(),
            &QNetworkAccessManager::authenticationRequired, this,
            [this, res](QNetworkReply *reply, QAuthenticator *authenticator) {
              if (res == reply) {
                authenticator->setUser(this->clientIdentifier());
                authenticator->setPassword("");
              }
            });

    connect(res, &QNetworkReply::finished, this, [this, res]() {
      if (res->error() != QNetworkReply::NoError) {
        emit this->revokeError(res->error(), res->errorString());
        return;
      }

      this->data()->setExpirationAt(QDateTime());
      this->data()->setToken(QString());

      if (!this->data()->refreshToken().isEmpty()) {
        QUrlQuery revokeData;
        revokeData.addQueryItem("token", this->data()->refreshToken());
        revokeData.addQueryItem("token_token_hint", "refresh_token");

        QNetworkReply *res = m_authFlow->networkAccessManager()->post(
            QNetworkRequest(QUrl("https://www.reddit.com/api/v1/revoke_token")),
            revokeData.toString(QUrl::FullyEncoded).toUtf8());

        connect(
            m_authFlow->networkAccessManager(),
            &QNetworkAccessManager::authenticationRequired, this,
            [this, res](QNetworkReply *reply, QAuthenticator *authenticator) {
              if (res == reply) {
                authenticator->setUser(this->clientIdentifier());
                authenticator->setPassword("");
              }
            });

        connect(res, &QNetworkReply::finished, this, [this, res]() {
          if (res->error() != QNetworkReply::NoError) {
            emit this->revokeError(res->error(), res->errorString());
            return;
          }

          this->data()->setRefreshToken(QString());
          emit this->revoked();
        });

      } else
        emit this->revoked();
    });
  }
}

void RedditAuthorization::setNetworkAccessManager(QNetworkAccessManager *nam) {
  m_authFlow->setNetworkAccessManager(nam);
}
