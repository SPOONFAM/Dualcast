#pragma once

#include "config/configuration.hpp"

#include <obs.h>
#include <callback/calldata.h>

#include <QObject>
#include <QTimer>

#include <array>
#include <memory>

namespace dualcast {

struct OutputSnapshot {
  bool active = false;
  bool reconnecting = false;
  int reconnectCount = 0;
  int bitrateKbps = 0;
  int droppedFrames = 0;
  int totalFrames = 0;
  int connectTimeMs = 0;
  float congestion = 0.0f;
  qint64 uptimeSec = 0;
  QString encoderName;
  QString lastError;
};

class OutputManager final : public QObject {
  Q_OBJECT
public:
  explicit OutputManager(QObject *parent = nullptr);
  ~OutputManager() override;

  bool start(Platform platform, const DualcastConfiguration &configuration, QString *error);
  bool startAll(const DualcastConfiguration &configuration, QString *error);
  void stop(Platform platform);
  void stopAll();
  bool isActive(Platform platform) const;
  OutputSnapshot snapshot(Platform platform) const;
  QString encoderPlan(const DualcastConfiguration &configuration) const;

signals:
  void statusChanged();
  void platformStarted(dualcast::Platform platform);
  void platformStopped(dualcast::Platform platform, int code);
  void platformReconnecting(dualcast::Platform platform, int attempt);

private:
  struct Slot;
  std::array<std::unique_ptr<Slot>, 2> slots_;
  QTimer pollTimer_;

  Slot *slot(Platform platform) const;
  bool createSlot(Platform platform, const DualcastConfiguration &configuration, QString *error);
  void releaseSlot(Platform platform);
  void poll();

  static void onStart(void *data, calldata_t *calldata);
  static void onStop(void *data, calldata_t *calldata);
  static void onReconnect(void *data, calldata_t *calldata);
  static void onReconnectSuccess(void *data, calldata_t *calldata);
};

} // namespace dualcast
