#pragma once

#include "config/configuration.hpp"

#include <QString>

namespace dualcast {

struct BandwidthEstimate {
  int videoKbps = 0;
  int audioKbps = 0;
  int overheadKbps = 0;
  int totalKbps = 0;
  bool capacityKnown = false;
  bool capacitySufficient = true;
  QString warning;
};

class BandwidthMonitor final {
public:
  static BandwidthEstimate estimate(const DualcastConfiguration &configuration);
};

} // namespace dualcast
