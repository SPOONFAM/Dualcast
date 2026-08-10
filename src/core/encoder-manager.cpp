#include "encoder-manager.hpp"

#include <obs.h>

namespace dualcast {
namespace {

QStringList availableEncoderIds()
{
  QStringList ids;
  for (size_t i = 0;; ++i) {
    const char *id = nullptr;
    if (!obs_enum_encoder_types(i, &id))
      break;
    if (id)
      ids.push_back(QString::fromUtf8(id));
  }
  return ids;
}

QString firstAvailable(const QStringList &candidates)
{
  const auto available = availableEncoderIds();
  for (const auto &candidate : candidates) {
    if (available.contains(candidate))
      return candidate;
  }
  return {};
}

} // namespace

QString EncoderManager::resolveVideoEncoderId(EncoderPreference preference)
{
  switch (preference) {
  case EncoderPreference::Nvenc: return firstAvailable({QStringLiteral("obs_nvenc_h264"), QStringLiteral("jim_nvenc")});
  case EncoderPreference::Amf: return firstAvailable({QStringLiteral("h264_texture_amf"), QStringLiteral("amf_h264")});
  case EncoderPreference::Qsv: return firstAvailable({QStringLiteral("obs_qsv11_h264"), QStringLiteral("obs_qsv11_v2")});
  case EncoderPreference::X264: return firstAvailable({QStringLiteral("obs_x264")});
  case EncoderPreference::Automatic:
    return firstAvailable({QStringLiteral("obs_nvenc_h264"), QStringLiteral("h264_texture_amf"), QStringLiteral("obs_qsv11_h264"), QStringLiteral("obs_x264")});
  }
  return {};
}

bool EncoderManager::isAvailable(const QString &id)
{
  return !id.isEmpty() && availableEncoderIds().contains(id);
}

QString EncoderManager::displayName(const QString &id)
{
  const auto name = obs_encoder_get_display_name(id.toUtf8().constData());
  return name ? QString::fromUtf8(name) : id;
}

obs_encoder_t *EncoderManager::createVideo(const QString &id, const DestinationConfig &configuration, const QString &name)
{
  obs_data_t *settings = obs_data_create();
  obs_data_set_string(settings, "rate_control", "CBR");
  obs_data_set_int(settings, "bitrate", configuration.bitrateKbps);
  obs_data_set_int(settings, "keyint_sec", configuration.keyframeIntervalSec);
  obs_encoder_t *encoder = obs_video_encoder_create(id.toUtf8().constData(), name.toUtf8().constData(), settings, nullptr);
  obs_data_release(settings);
  if (encoder)
    obs_encoder_set_scaled_size(encoder, static_cast<uint32_t>(configuration.width), static_cast<uint32_t>(configuration.height));
  return encoder;
}

obs_encoder_t *EncoderManager::createAudio(const DestinationConfig &configuration, const QString &name)
{
  obs_data_t *settings = obs_data_create();
  obs_data_set_int(settings, "bitrate", configuration.audioBitrateKbps);
  obs_encoder_t *encoder = obs_audio_encoder_create("ffmpeg_aac", name.toUtf8().constData(), settings, 0, nullptr);
  obs_data_release(settings);
  return encoder;
}

bool EncoderManager::canShare(const DestinationConfig &a, const DestinationConfig &b)
{
  return a.width == b.width && a.height == b.height && a.fps == b.fps && a.bitrateKbps == b.bitrateKbps &&
         a.keyframeIntervalSec == b.keyframeIntervalSec && a.encoder == b.encoder;
}

} // namespace dualcast
