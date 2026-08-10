#pragma once

#include "core/output-manager.hpp"

#include <QWidget>

namespace dualcast {

class StreamHealthWidget final : public QWidget {
  Q_OBJECT
public:
  explicit StreamHealthWidget(Platform platform, QWidget *parent = nullptr);
  void setSnapshot(const OutputSnapshot &snapshot);

private:
  Platform platform_;
  class QLabel *state_ = nullptr;
  class QLabel *metrics_ = nullptr;
};

} // namespace dualcast
