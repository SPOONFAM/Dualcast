#include "output-manager.hpp"

#include "auth/secure-storage.hpp"
#include "core/bandwidth-monitor.hpp"
#include "core/encoder-manager.hpp"

#include <obs.h>
#include <util/callback/signal.h>

#include <QDateTime>
#include <QMetaObject>

#include <utility>

namespace dualcast {

struct OutputManager::Slot {
  OutputManager *manager = nullptr;
  Platform platform = Platform::YouTube;
  obs_service_t *service = nullptr;
  obs_output_t *output = nullptr;
  obs_encoder_t *videoEncoder = nullptr;
  obs_encoder_t *audioEncoder = nullptr;
  QString encoderName;
  qint64 startedAt = 0;
  quint64 lastBytes = 0;
  qint64 lastSampleMs = 0;
  OutputSnapshot snapshot;
};

namespace {

int indexFor(Platform platform)
{
  return platform == Platform::YouTube ? 0 : 1;
}

void logPlatform(Platform platform, const char *message)
{
  obs_log(LOG_INFO, "[Dualcast] %s %s", platform == Platform::YouTube ? "YouTube" : "TikTok", message);
}

void queueStatus(OutputManager *manager)
{
  QMetaObject::invokeMethod(manager, [manager] { emit manager->statusChanged(); }, Qt::QueuedConnection);
}

} // namespace

OutputManager::OutputManager(QObject *parent) : QObject(parent)
{
  connect(&pollTimer_, &QTimer::timeout, this, &OutputManager::poll);
  pollTimer_.start(1000);
}

OutputManager::~OutputManager()
{
  stopAll();
  for (int i = 0; i < 2; ++i)
    releaseSlot(i == 0 ? Platform::YouTube : Platform::TikTok);
}

OutputManager::Slot *OutputManager::slot(Platform platform) const
{
  return slots_[indexFor(platform)].get();
}

bool OutputManager::createSlot(Platform platform, const DualcastConfiguration &configuration, QString *error)
{
  const auto &destinationConfig = dualcast::destination(configuration, platform);
  if (!destinationConfig.enabled || !destinationConfig.configured) {
    if (error) *error = QStringLiteral("%1 is not configured.").arg(platformName(platform));
    return false;
  }
  if (destinationConfig.server.isEmpty() || (!destinationConfig.server.startsWith("rtmp://") && !destinationConfig.server.startsWith("rtmps://"))) {
    if (error) *error = QStringLiteral("%1 requires an RTMP or RTMPS server URL.").arg(platformName(platform));
    return false;
  }
  const double programFps = obs_get_video() ? video_output_get_frame_rate(obs_get_video()) : 0.0;
  if (programFps > 0.0 && qAbs(programFps - destinationConfig.fps) > 0.01) {
    if (error) *error = QStringLiteral("%1 is set to %2 FPS, but Dualcast uses OBS's current program output at %3 FPS. Change the destination FPS or the OBS Video FPS before starting.").arg(platformName(platform)).arg(destinationConfig.fps).arg(programFps, 0, 'f', 2);
    return false;
  }

  QString key;
  if (!SecureStorage::load(destinationConfig.secretId.isEmpty() ? ConfigurationStore::secretId(platform) : destinationConfig.secretId, &key, error))
    return false;

  auto newSlot = std::make_unique<Slot>();
  newSlot->manager = this;
  newSlot->platform = platform;
  obs_data_t *serviceSettings = obs_data_create();
  obs_data_set_string(serviceSettings, "server", destinationConfig.server.toUtf8().constData());
  obs_data_set_string(serviceSettings, "key", key.toUtf8().constData());
  newSlot->service = obs_service_create("rtmp_custom", QStringLiteral("dualcast_%1_service").arg(platformName(platform)).toUtf8().constData(), serviceSettings, nullptr);
  obs_data_release(serviceSettings);
  if (!newSlot->service) {
    if (error) *error = QStringLiteral("OBS could not create the RTMP service for %1.").arg(platformName(platform));
    return false;
  }

  newSlot->output = obs_output_create("rtmp_output", QStringLiteral("dualcast_%1_output").arg(platformName(platform)).toUtf8().constData(), nullptr, nullptr);
  if (!newSlot->output) {
    obs_service_release(newSlot->service);
    newSlot->service = nullptr;
    if (error) *error = QStringLiteral("OBS could not create the RTMP output for %1.").arg(platformName(platform));
    return false;
  }
  obs_output_set_service(newSlot->output, newSlot->service);
  obs_output_set_reconnect_settings(newSlot->output, destinationConfig.reconnectCount, destinationConfig.reconnectDelaySec);
  obs_output_set_preferred_size(newSlot->output, static_cast<uint32_t>(destinationConfig.width), static_cast<uint32_t>(destinationConfig.height));

  const auto encoderId = EncoderManager::resolveVideoEncoderId(destinationConfig.encoder);
  if (encoderId.isEmpty()) {
    if (error) *error = QStringLiteral("No supported H.264 video encoder is available for %1.").arg(platformName(platform));
    obs_output_release(newSlot->output);
    obs_service_release(newSlot->service);
    return false;
  }
  newSlot->videoEncoder = EncoderManager::createVideo(encoderId, destinationConfig, QStringLiteral("Dualcast %1 video").arg(platformName(platform)));
  newSlot->audioEncoder = EncoderManager::createAudio(destinationConfig, QStringLiteral("Dualcast %1 audio").arg(platformName(platform)));
  if (!newSlot->videoEncoder || !newSlot->audioEncoder) {
    if (error) *error = QStringLiteral("OBS could not initialize encoders for %1.").arg(platformName(platform));
    if (newSlot->videoEncoder) obs_encoder_release(newSlot->videoEncoder);
    if (newSlot->audioEncoder) obs_encoder_release(newSlot->audioEncoder);
    obs_output_release(newSlot->output);
    obs_service_release(newSlot->service);
    return false;
  }
  obs_encoder_set_video(newSlot->videoEncoder, obs_get_video());
  obs_encoder_set_audio(newSlot->audioEncoder, obs_get_audio());
  obs_output_set_video_encoder(newSlot->output, newSlot->videoEncoder);
  obs_output_set_audio_encoder(newSlot->output, newSlot->audioEncoder, 0);
  newSlot->encoderName = EncoderManager::displayName(encoderId);

  signal_handler_t *signals = obs_output_get_signal_handler(newSlot->output);
  signal_handler_connect(signals, "start", &OutputManager::onStart, newSlot.get());
  signal_handler_connect(signals, "stop", &OutputManager::onStop, newSlot.get());
  signal_handler_connect(signals, "reconnect", &OutputManager::onReconnect, newSlot.get());
  signal_handler_connect(signals, "reconnect_success", &OutputManager::onReconnectSuccess, newSlot.get());

  slots_[indexFor(platform)] = std::move(newSlot);
  return true;
}

bool OutputManager::start(Platform platform, const DualcastConfiguration &configuration, QString *error)
{
  if (isActive(platform))
    return true;
  if (slot(platform))
    releaseSlot(platform);
  if (!createSlot(platform, configuration, error))
    return false;
  auto *current = slot(platform);
  if (!obs_output_start(current->output)) {
    if (error) {
      const char *lastError = obs_output_get_last_error(current->output);
      *error = lastError ? QString::fromUtf8(lastError) : QStringLiteral("OBS could not start the %1 output.").arg(platformName(platform));
    }
    releaseSlot(platform);
    return false;
  }
  current->startedAt = QDateTime::currentMSecsSinceEpoch();
  logPlatform(platform, "output initialized");
  return true;
}

bool OutputManager::startAll(const DualcastConfiguration &configuration, QString *error)
{
  const auto estimate = BandwidthMonitor::estimate(configuration);
  if (estimate.capacityKnown && !estimate.capacitySufficient) {
    if (error) *error = estimate.warning;
    return false;
  }
  bool any = false;
  QString firstError;
  for (const auto platform : {Platform::YouTube, Platform::TikTok}) {
    if (!destination(configuration, platform).enabled)
      continue;
    any = true;
    QString platformError;
    if (!start(platform, configuration, &platformError) && firstError.isEmpty())
      firstError = platformError;
  }
  if (!any) {
    if (error) *error = QStringLiteral("Select at least one enabled destination.");
    return false;
  }
  if (!firstError.isEmpty()) {
    if (error) *error = firstError;
    return false;
  }
  if (error) *error = firstError;
  return true;
}

void OutputManager::stop(Platform platform)
{
  if (auto *current = slot(platform); current && current->output && obs_output_active(current->output))
    obs_output_stop(current->output);
}

void OutputManager::stopAll()
{
  stop(Platform::YouTube);
  stop(Platform::TikTok);
}

void OutputManager::releaseSlot(Platform platform)
{
  auto &target = slots_[indexFor(platform)];
  if (!target)
    return;
  if (target->output && obs_output_active(target->output))
    obs_output_force_stop(target->output);
  if (target->output) {
    auto *signals = obs_output_get_signal_handler(target->output);
    signal_handler_disconnect(signals, "start", &OutputManager::onStart, target.get());
    signal_handler_disconnect(signals, "stop", &OutputManager::onStop, target.get());
    signal_handler_disconnect(signals, "reconnect", &OutputManager::onReconnect, target.get());
    signal_handler_disconnect(signals, "reconnect_success", &OutputManager::onReconnectSuccess, target.get());
  }
  if (target->videoEncoder) obs_encoder_release(target->videoEncoder);
  if (target->audioEncoder) obs_encoder_release(target->audioEncoder);
  if (target->output) obs_output_release(target->output);
  if (target->service) obs_service_release(target->service);
  target.reset();
}

bool OutputManager::isActive(Platform platform) const
{
  const auto *current = slot(platform);
  return current && current->output && obs_output_active(current->output);
}

OutputSnapshot OutputManager::snapshot(Platform platform) const
{
  const auto *current = slot(platform);
  return current ? current->snapshot : OutputSnapshot{};
}

QString OutputManager::encoderPlan(const DualcastConfiguration &configuration) const
{
  if (configuration.youtube.enabled && configuration.tiktok.enabled && EncoderManager::canShare(configuration.youtube, configuration.tiktok))
    return QStringLiteral("Compatible settings detected; OBS public API still requires independent encoder contexts.");
  return QStringLiteral("Independent encoders");
}

void OutputManager::poll()
{
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  for (auto &current : slots_) {
    if (!current || !current->output)
      continue;
    current->snapshot.active = obs_output_active(current->output);
    current->snapshot.reconnecting = obs_output_reconnecting(current->output);
    current->snapshot.droppedFrames = obs_output_get_frames_dropped(current->output);
    current->snapshot.totalFrames = obs_output_get_total_frames(current->output);
    current->snapshot.connectTimeMs = obs_output_get_connect_time_ms(current->output);
    current->snapshot.congestion = obs_output_get_congestion(current->output);
    current->snapshot.encoderName = current->encoderName;
    const quint64 bytes = obs_output_get_total_bytes(current->output);
    if (current->lastSampleMs > 0 && now > current->lastSampleMs)
      current->snapshot.bitrateKbps = static_cast<int>((bytes - current->lastBytes) * 8 / (now - current->lastSampleMs));
    current->lastBytes = bytes;
    current->lastSampleMs = now;
    if (current->startedAt > 0)
      current->snapshot.uptimeSec = (now - current->startedAt) / 1000;
  }
  emit statusChanged();
}

void OutputManager::onStart(void *data, calldata_t *)
{
  auto *current = static_cast<Slot *>(data);
  current->snapshot.active = true;
  logPlatform(current->platform, "connected");
  const auto platform = current->platform;
  auto *manager = current->manager;
  QMetaObject::invokeMethod(manager, [manager, platform] { emit manager->platformStarted(platform); emit manager->statusChanged(); }, Qt::QueuedConnection);
}

void OutputManager::onStop(void *data, calldata_t *calldata)
{
  auto *current = static_cast<Slot *>(data);
  const int code = calldata_int(calldata, "code");
  current->snapshot.active = false;
  const char *lastError = current->output ? obs_output_get_last_error(current->output) : nullptr;
  current->snapshot.lastError = lastError ? QString::fromUtf8(lastError) : QString{};
  logPlatform(current->platform, code == OBS_OUTPUT_SUCCESS ? "stopped" : "connection lost");
  const auto platform = current->platform;
  auto *manager = current->manager;
  QMetaObject::invokeMethod(manager, [manager, platform, code] { emit manager->platformStopped(platform, code); emit manager->statusChanged(); }, Qt::QueuedConnection);
}

void OutputManager::onReconnect(void *data, calldata_t *)
{
  auto *current = static_cast<Slot *>(data);
  ++current->snapshot.reconnectCount;
  logPlatform(current->platform, "reconnect attempt");
  const auto platform = current->platform;
  const auto attempt = current->snapshot.reconnectCount;
  auto *manager = current->manager;
  QMetaObject::invokeMethod(manager, [manager, platform, attempt] { emit manager->platformReconnecting(platform, attempt); emit manager->statusChanged(); }, Qt::QueuedConnection);
}

void OutputManager::onReconnectSuccess(void *data, calldata_t *)
{
  auto *current = static_cast<Slot *>(data);
  logPlatform(current->platform, "reconnected");
  queueStatus(current->manager);
}

} // namespace dualcast
