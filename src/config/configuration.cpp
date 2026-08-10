#include "configuration.hpp"

#include <obs-frontend-api.h>
#include <util/config-file.h>

namespace dualcast {
namespace {

constexpr const char *kSection = "Dualcast";

QString readString(config_t *config, const char *key, const QString &fallback = {})
{
  const char *value = config_get_string(config, kSection, key);
  return value ? QString::fromUtf8(value) : fallback;
}

void writeDestination(config_t *config, const char *prefix, const DestinationConfig &d)
{
  const auto key = [prefix](const char *suffix) { return QStringLiteral("%1_%2").arg(QString::fromUtf8(prefix), QString::fromUtf8(suffix)).toUtf8(); };
  config_set_bool(config, kSection, key("enabled").constData(), d.enabled);
  config_set_bool(config, kSection, key("configured").constData(), d.configured);
  config_set_bool(config, kSection, key("oauth").constData(), d.oauthConnected);
  config_set_string(config, kSection, key("account").constData(), d.accountName.toUtf8().constData());
  config_set_string(config, kSection, key("avatar").constData(), d.accountAvatarUrl.toUtf8().constData());
  config_set_string(config, kSection, key("server").constData(), d.server.toUtf8().constData());
  config_set_string(config, kSection, key("secret_id").constData(), d.secretId.toUtf8().constData());
  config_set_int(config, kSection, key("width").constData(), d.width);
  config_set_int(config, kSection, key("height").constData(), d.height);
  config_set_int(config, kSection, key("fps").constData(), d.fps);
  config_set_int(config, kSection, key("bitrate_kbps").constData(), d.bitrateKbps);
  config_set_int(config, kSection, key("audio_kbps").constData(), d.audioBitrateKbps);
  config_set_int(config, kSection, key("keyframe_sec").constData(), d.keyframeIntervalSec);
  config_set_int(config, kSection, key("encoder").constData(), static_cast<int>(d.encoder));
  config_set_bool(config, kSection, key("automatic_quality").constData(), d.automaticQuality);
  config_set_int(config, kSection, key("reconnect_count").constData(), d.reconnectCount);
  config_set_int(config, kSection, key("reconnect_delay_sec").constData(), d.reconnectDelaySec);
}

DestinationConfig readDestination(config_t *config, const char *prefix)
{
  DestinationConfig d;
  const auto key = [prefix](const char *suffix) { return QStringLiteral("%1_%2").arg(QString::fromUtf8(prefix), QString::fromUtf8(suffix)).toUtf8(); };
  d.enabled = config_get_bool(config, kSection, key("enabled").constData());
  d.configured = config_get_bool(config, kSection, key("configured").constData());
  d.oauthConnected = config_get_bool(config, kSection, key("oauth").constData());
  d.accountName = readString(config, key("account").constData());
  d.accountAvatarUrl = readString(config, key("avatar").constData());
  d.server = readString(config, key("server").constData());
  d.secretId = readString(config, key("secret_id").constData());
  d.width = config_get_int(config, kSection, key("width").constData());
  d.height = config_get_int(config, kSection, key("height").constData());
  d.fps = config_get_int(config, kSection, key("fps").constData());
  d.bitrateKbps = config_get_int(config, kSection, key("bitrate_kbps").constData());
  d.audioBitrateKbps = config_get_int(config, kSection, key("audio_kbps").constData());
  d.keyframeIntervalSec = config_get_int(config, kSection, key("keyframe_sec").constData());
  d.encoder = static_cast<EncoderPreference>(config_get_int(config, kSection, key("encoder").constData()));
  d.automaticQuality = config_get_bool(config, kSection, key("automatic_quality").constData());
  d.reconnectCount = config_get_int(config, kSection, key("reconnect_count").constData());
  d.reconnectDelaySec = config_get_int(config, kSection, key("reconnect_delay_sec").constData());

  if (d.width <= 0) d.width = 1920;
  if (d.height <= 0) d.height = 1080;
  if (d.fps <= 0) d.fps = 60;
  if (d.bitrateKbps <= 0) d.bitrateKbps = 8000;
  if (d.audioBitrateKbps <= 0) d.audioBitrateKbps = 160;
  if (d.keyframeIntervalSec <= 0) d.keyframeIntervalSec = 2;
  if (d.reconnectCount < 0) d.reconnectCount = 5;
  if (d.reconnectDelaySec <= 0) d.reconnectDelaySec = 2;
  return d;
}

} // namespace

DualcastConfiguration ConfigurationStore::load()
{
  DualcastConfiguration result;
  config_t *config = obs_frontend_get_profile_config();
  if (!config)
    return result;

  result.outputMode = config_get_int(config, kSection, "output_mode") == 1 ? OutputMode::PlatformLayout : OutputMode::Mirror;
  result.startWithObs = config_get_bool(config, kSection, "start_with_obs");
  const char *firstRun = config_get_string(config, kSection, "first_run");
  result.firstRun = !firstRun || config_get_bool(config, kSection, "first_run");
  result.knownUploadMbps = config_get_int(config, kSection, "known_upload_mbps");
  result.youtube = readDestination(config, "youtube");
  result.tiktok = readDestination(config, "tiktok");
  return result;
}

void ConfigurationStore::save(const DualcastConfiguration &configuration)
{
  config_t *config = obs_frontend_get_profile_config();
  if (!config)
    return;
  config_set_int(config, kSection, "output_mode", configuration.outputMode == OutputMode::PlatformLayout ? 1 : 0);
  config_set_bool(config, kSection, "start_with_obs", configuration.startWithObs);
  config_set_bool(config, kSection, "first_run", configuration.firstRun);
  config_set_int(config, kSection, "known_upload_mbps", configuration.knownUploadMbps);
  writeDestination(config, "youtube", configuration.youtube);
  writeDestination(config, "tiktok", configuration.tiktok);
  config_save(config);
}

QString ConfigurationStore::secretId(Platform platform)
{
  return platform == Platform::YouTube ? QStringLiteral("youtube_stream_key") : QStringLiteral("tiktok_stream_key");
}

const DestinationConfig &destination(const DualcastConfiguration &c, Platform p)
{
  return p == Platform::YouTube ? c.youtube : c.tiktok;
}

DestinationConfig &destination(DualcastConfiguration &c, Platform p)
{
  return p == Platform::YouTube ? c.youtube : c.tiktok;
}

QString platformName(Platform platform)
{
  return platform == Platform::YouTube ? QStringLiteral("YouTube") : QStringLiteral("TikTok");
}

} // namespace dualcast
