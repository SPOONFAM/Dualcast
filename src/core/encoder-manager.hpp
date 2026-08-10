#pragma once

#include "config/configuration.hpp"

#include <obs.h>

#include <QString>
#include <QStringList>

namespace dualcast {

class EncoderManager final {
public:
  static QString resolveVideoEncoderId(EncoderPreference preference);
  static bool isAvailable(const QString &id);
  static QString displayName(const QString &id);
  static obs_encoder_t *createVideo(const QString &id, const DestinationConfig &configuration, const QString &name);
  static obs_encoder_t *createAudio(const DestinationConfig &configuration, const QString &name);
  static bool canShare(const DestinationConfig &a, const DestinationConfig &b);
};

} // namespace dualcast
