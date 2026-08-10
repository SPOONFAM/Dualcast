#pragma once

#include "config/configuration.hpp"

#include <QString>

namespace dualcast {

struct ProviderCapabilities {
  bool oauth = false;
  bool automaticIngest = false;
  bool manualRtmp = true;
  bool broadcastManagement = false;
};

struct AccountIdentity {
  QString displayName;
  QString avatarUrl;
  bool connected = false;
};

struct ProviderIngestConfiguration {
  bool available = false;
  QString server;
  QString secretId;
};

struct ProviderHealth {
  bool connected = false;
  QString message;
};

class StreamingProvider {
public:
  virtual ~StreamingProvider() = default;
  virtual Platform platform() const = 0;
  virtual ProviderCapabilities capabilities() const = 0;
  virtual AccountIdentity account() const = 0;
  virtual bool authenticate(QString *error) = 0;
  virtual void disconnect() = 0;
  virtual ProviderIngestConfiguration getIngestConfiguration() const = 0;
  virtual bool startBroadcast(QString *error)
  {
    if (error) *error = QStringLiteral("This provider does not expose broadcast management through its official API.");
    return false;
  }
  virtual bool stopBroadcast(QString *error)
  {
    if (error) *error = QStringLiteral("This provider does not expose broadcast management through its official API.");
    return false;
  }
  virtual ProviderHealth getHealth() const { return {.connected = account().connected, .message = {}}; }
};

} // namespace dualcast
