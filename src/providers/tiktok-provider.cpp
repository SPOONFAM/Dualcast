#include "tiktok-provider.hpp"

namespace dualcast {

ProviderCapabilities TikTokProvider::capabilities() const
{
  // Login Kit identity and LIVE ingest authorization are intentionally separate.
  return {.oauth = false, .automaticIngest = false, .manualRtmp = true, .broadcastManagement = false};
}

bool TikTokProvider::authenticate(QString *error)
{
  if (error)
    *error = QStringLiteral("TikTok Login Kit is not enabled by a configured official provider. Configure LIVE ingest manually.");
  return false;
}

void TikTokProvider::disconnect()
{
  account_ = {};
}

ProviderIngestConfiguration TikTokProvider::getIngestConfiguration() const
{
  return {};
}

} // namespace dualcast
