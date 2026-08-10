#include "oauth-manager.hpp"

#include "auth/secure-storage.hpp"

#include <QDesktopServices>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QRandomGenerator>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QUrlQuery>

namespace dualcast {
namespace {

QString randomState()
{
  return QString::number(QRandomGenerator::global()->generate64(), 16) + QString::number(QRandomGenerator::global()->generate64(), 16);
}

} // namespace

OAuthManager::OAuthManager(QObject *parent) : QObject(parent) {}

std::optional<OAuthAccountResult> OAuthManager::startGoogleFlow(QString *error)
{
#ifndef DUALCAST_GOOGLE_CLIENT_ID
  if (error) *error = QStringLiteral("Dualcast was built without a Google OAuth client id.");
  return std::nullopt;
#else
  const QString clientId = QStringLiteral(DUALCAST_GOOGLE_CLIENT_ID);
  if (clientId.isEmpty()) {
    if (error) *error = QStringLiteral("Configure DUALCAST_GOOGLE_CLIENT_ID when building Dualcast to enable YouTube OAuth.");
    return std::nullopt;
  }

  QTcpServer server;
  if (!server.listen(QHostAddress::LocalHost, 0)) {
    if (error) *error = QStringLiteral("Could not open a local OAuth callback port.");
    return std::nullopt;
  }
  const QString state = randomState();
  QUrl url(QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth"));
  QUrlQuery query;
  query.addQueryItem(QStringLiteral("client_id"), clientId);
  query.addQueryItem(QStringLiteral("redirect_uri"), QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort()));
  query.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
  query.addQueryItem(QStringLiteral("scope"), QStringLiteral("openid profile https://www.googleapis.com/auth/youtube.readonly"));
  query.addQueryItem(QStringLiteral("access_type"), QStringLiteral("offline"));
  query.addQueryItem(QStringLiteral("prompt"), QStringLiteral("consent"));
  query.addQueryItem(QStringLiteral("state"), state);
  url.setQuery(query);
  if (!QDesktopServices::openUrl(url)) {
    if (error) *error = QStringLiteral("Could not open the browser for Google authorization.");
    return std::nullopt;
  }

  if (!server.waitForNewConnection(120000)) {
    if (error) *error = QStringLiteral("Timed out waiting for Google authorization.");
    return std::nullopt;
  }
  auto *socket = server.nextPendingConnection();
  socket->waitForReadyRead(2000);
  const auto request = QString::fromUtf8(socket->readAll());
  const auto firstLine = request.section('\n', 0, 0).trimmed();
  const auto target = firstLine.section(' ', 1, 1);
  QUrl callback(QStringLiteral("http://127.0.0.1") + target);
  QUrlQuery callbackQuery(callback);
  const auto returnedState = callbackQuery.queryItemValue(QStringLiteral("state"));
  const auto code = callbackQuery.queryItemValue(QStringLiteral("code"));
  socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nYouTube is connected. You may close this window.");
  socket->disconnectFromHost();
  if (returnedState != state || code.isEmpty()) {
    if (error) *error = QStringLiteral("Google authorization was rejected or returned an invalid state.");
    return std::nullopt;
  }

  QNetworkRequest tokenRequest(QUrl(QStringLiteral("https://oauth2.googleapis.com/token")));
  tokenRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
  QUrlQuery body;
  body.addQueryItem(QStringLiteral("client_id"), clientId);
  body.addQueryItem(QStringLiteral("code"), code);
  body.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
  body.addQueryItem(QStringLiteral("redirect_uri"), QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort()));
  auto *reply = network_.post(tokenRequest, body.toString(QUrl::FullyEncoded).toUtf8());
  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();
  const auto tokenJson = QJsonDocument::fromJson(reply->readAll()).object();
  const auto accessToken = tokenJson.value(QStringLiteral("access_token")).toString();
  const auto refreshToken = tokenJson.value(QStringLiteral("refresh_token")).toString();
  if (accessToken.isEmpty()) {
    if (error) *error = QStringLiteral("Google did not return an access token.");
    reply->deleteLater();
    return std::nullopt;
  }

  QNetworkRequest profileRequest(QUrl(QStringLiteral("https://www.googleapis.com/youtube/v3/channels?part=snippet&mine=true")));
  profileRequest.setRawHeader("Authorization", QByteArray("Bearer ") + accessToken.toUtf8());
  auto *profileReply = network_.get(profileRequest);
  QObject::connect(profileReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();
  const auto profile = QJsonDocument::fromJson(profileReply->readAll()).object();
  const auto items = profile.value(QStringLiteral("items")).toArray();
  const auto snippet = items.isEmpty() ? QJsonObject{} : items.first().toObject().value(QStringLiteral("snippet")).toObject();
  const auto thumbnails = snippet.value(QStringLiteral("thumbnails")).toObject();
  const auto defaultThumbnail = thumbnails.value(QStringLiteral("default")).toObject();
  OAuthAccountResult result{snippet.value(QStringLiteral("title")).toString(), defaultThumbnail.value(QStringLiteral("url")).toString(), accessToken, refreshToken};
  QString storageError;
  if (!SecureStorage::store(QStringLiteral("youtube_oauth_access"), accessToken, &storageError) ||
      (!refreshToken.isEmpty() && !SecureStorage::store(QStringLiteral("youtube_oauth_refresh"), refreshToken, &storageError))) {
    if (error) *error = storageError;
    return std::nullopt;
  }
  reply->deleteLater();
  profileReply->deleteLater();
  return result;
#endif
}

void OAuthManager::clearTokens()
{
  SecureStorage::remove(QStringLiteral("youtube_oauth_access"));
  SecureStorage::remove(QStringLiteral("youtube_oauth_refresh"));
}

} // namespace dualcast
