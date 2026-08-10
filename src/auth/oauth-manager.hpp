#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <optional>

namespace dualcast {

struct OAuthAccountResult {
  QString accountName;
  QString avatarUrl;
  QString accessToken;
  QString refreshToken;
};

class OAuthManager final : public QObject {
  Q_OBJECT
public:
  explicit OAuthManager(QObject *parent = nullptr);
  std::optional<OAuthAccountResult> startGoogleFlow(QString *error);
  void clearTokens();

private:
  QNetworkAccessManager network_;
};

} // namespace dualcast
