#pragma once

#include "providers/streaming-provider.hpp"

namespace dualcast {

class TikTokProvider final : public StreamingProvider {
public:
  Platform platform() const override { return Platform::TikTok; }
  ProviderCapabilities capabilities() const override;
  AccountIdentity account() const override { return account_; }
  bool authenticate(QString *error) override;
  void disconnect() override;
  ProviderIngestConfiguration getIngestConfiguration() const override;

private:
  AccountIdentity account_;
};

} // namespace dualcast
