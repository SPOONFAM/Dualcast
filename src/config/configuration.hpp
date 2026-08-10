#pragma once

#include <QString>

#include <array>

namespace dualcast {

enum class Platform { YouTube = 0, TikTok = 1 };
enum class OutputMode { Mirror, PlatformLayout };
enum class EncoderPreference { Automatic, Nvenc, Amf, Qsv, X264 };

struct DestinationConfig {
  bool enabled = false;
  bool configured = false;
  bool oauthConnected = false;
  QString accountName;
  QString accountAvatarUrl;
  QString server;
  QString secretId;
  int width = 1920;
  int height = 1080;
  int fps = 60;
  int bitrateKbps = 8000;
  int audioBitrateKbps = 160;
  int keyframeIntervalSec = 2;
  EncoderPreference encoder = EncoderPreference::Automatic;
  bool automaticQuality = true;
  int reconnectCount = 5;
  int reconnectDelaySec = 2;
};

struct DualcastConfiguration {
  OutputMode outputMode = OutputMode::Mirror;
  bool startWithObs = false;
  bool firstRun = true;
  int knownUploadMbps = 0;
  DestinationConfig youtube;
  DestinationConfig tiktok;
};

class ConfigurationStore final {
public:
  static DualcastConfiguration load();
  static void save(const DualcastConfiguration &configuration);
  static QString secretId(Platform platform);
};

const DestinationConfig &destination(const DualcastConfiguration &, Platform);
DestinationConfig &destination(DualcastConfiguration &, Platform);
QString platformName(Platform);

} // namespace dualcast
