#ifndef REDDITAUTHORIZATIONDATA_HXX
#define REDDITAUTHORIZATIONDATA_HXX

#include <QDateTime>
#include <QObject>
#include <QString>

class RedditAuthorizationData : public QObject {
private:
  QDateTime m_expirationAt;
  QString m_refreshToken;
  QString m_token;

public:
  RedditAuthorizationData(QDateTime expirationAt, QString token,
                          QString refreshToken = QString(),
                          QObject *parent = nullptr);
  RedditAuthorizationData(QObject *parent = nullptr);
  bool expired() const;
  QDateTime expirationAt() const;
  QString refreshToken() const;
  QString token() const;
  void setToken(QString token);
  void setRefreshToken(QString refreshToken);
  void setExpirationAt(QDateTime expirationAt);
};

#endif // REDDITAUTHORIZATIONDATA_HXX
