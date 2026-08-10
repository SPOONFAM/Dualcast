#include "core/bandwidth-monitor.hpp"

#include <cassert>

int main()
{
  dualcast::DualcastConfiguration configuration;
  configuration.youtube.enabled = true;
  configuration.youtube.configured = true;
  configuration.youtube.bitrateKbps = 8000;
  configuration.youtube.audioBitrateKbps = 160;
  configuration.tiktok.enabled = true;
  configuration.tiktok.configured = true;
  configuration.tiktok.bitrateKbps = 6000;
  configuration.tiktok.audioBitrateKbps = 128;
  const auto estimate = dualcast::BandwidthMonitor::estimate(configuration);
  assert(estimate.videoKbps == 14000);
  assert(estimate.audioKbps == 288);
  assert(estimate.totalKbps > 14000);

  configuration.knownUploadMbps = 10;
  const auto constrained = dualcast::BandwidthMonitor::estimate(configuration);
  assert(constrained.capacityKnown);
  assert(!constrained.capacitySufficient);
  return 0;
}
