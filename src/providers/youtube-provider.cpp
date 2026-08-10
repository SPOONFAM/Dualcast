#include "youtube-provider.hpp"

namespace dualcast {

YouTubeProvider::YouTubeProvider(OAuthManager *oauth) : oauth_(oauth) {}

ProviderCapabilities YouTubeProvider::capabilities() const
{
  return {.oauth = true, .automaticIngest = false, .manualRtmp = true, .broadcastManagement = false};
}

bool YouTubeProvider::authenticate(QString *error)
{
  if (!oauth_)
    return false;
  const auto result = oauth_->startGoogleFlow(error);
  if (!result)
    return false;
  account_.connected = true;
  account_.displayName = result->accountName;
  account_.avatarUrl = result->avatarUrl;
  return true;
}

void YouTubeProvider::disconnect()
{
  account_ = {};
  if (oauth_)
    oauth_->clearTokens();
}

ProviderIngestConfiguration YouTubeProvider::getIngestConfiguration() const
{
  // OAuth account identity is available, but this implementation does not
  // claim that an OAuth token is an ingest credential.
  return {};
}

} // namespace dualcast
