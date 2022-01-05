#include "RedditAuthorizationData.hxx"

RedditAuthorizationData::RedditAuthorizationData(QDateTime expirationAt,
                                                 QString token,
                                                 QString refreshToken,
                                                 QObject *parent)
    : QObject(parent) {
  m_expirationAt = expirationAt;
  m_token = token;
  m_refreshToken = refreshToken;
}

RedditAuthorizationData::RedditAuthorizationData(QObject *parent)
    : RedditAuthorizationData(QDateTime(), QString(), QString(), parent) {}

bool RedditAuthorizationData::expired() const {
  if (m_expirationAt.isValid())
    return m_expirationAt <= QDateTime::currentDateTime();

  return true;
}

QDateTime RedditAuthorizationData::expirationAt() const {
  return m_expirationAt;
}

QString RedditAuthorizationData::refreshToken() const { return m_refreshToken; }

QString RedditAuthorizationData::token() const { return m_token; }

void RedditAuthorizationData::setToken(QString token) { m_token = token; }

void RedditAuthorizationData::setRefreshToken(QString refreshToken) {
  m_refreshToken = refreshToken;
}

void RedditAuthorizationData::setExpirationAt(QDateTime expirationAt) {
  m_expirationAt = expirationAt;
}
