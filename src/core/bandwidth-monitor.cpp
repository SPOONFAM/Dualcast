#include "bandwidth-monitor.hpp"

#include <algorithm>

namespace dualcast {

BandwidthEstimate BandwidthMonitor::estimate(const DualcastConfiguration &configuration)
{
  BandwidthEstimate result;
  const auto add = [&result](const DestinationConfig &d) {
    if (!d.enabled || !d.configured)
      return;
    result.videoKbps += std::max(0, d.bitrateKbps);
    result.audioKbps += std::max(0, d.audioBitrateKbps);
  };
  add(configuration.youtube);
  add(configuration.tiktok);
  result.overheadKbps = result.videoKbps > 0 ? std::max(128, result.videoKbps / 20) : 0;
  result.totalKbps = result.videoKbps + result.audioKbps + result.overheadKbps;
  if (configuration.knownUploadMbps > 0) {
    result.capacityKnown = true;
    result.capacitySufficient = result.totalKbps < configuration.knownUploadMbps * 1000 * 8 / 10;
    if (!result.capacitySufficient)
      result.warning = QStringLiteral("Estimated upload is within 20% of the configured upload capacity.");
  } else if (result.totalKbps > 0) {
    result.warning = QStringLiteral("Upload capacity has not been measured; verify that your connection has headroom.");
  }
  return result;
}

} // namespace dualcast
