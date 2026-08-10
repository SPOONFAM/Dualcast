#pragma once

#include "providers/streaming-provider.hpp"

#include "auth/oauth-manager.hpp"

namespace dualcast {

class YouTubeProvider final : public StreamingProvider {
public:
  explicit YouTubeProvider(OAuthManager *oauth);
  Platform platform() const override { return Platform::YouTube; }
  ProviderCapabilities capabilities() const override;
  AccountIdentity account() const override { return account_; }
  bool authenticate(QString *error) override;
  void disconnect() override;
  ProviderIngestConfiguration getIngestConfiguration() const override;

private:
  OAuthManager *oauth_ = nullptr;
  AccountIdentity account_;
};

} // namespace dualcast
