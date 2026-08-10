#pragma once

#include "core/output-manager.hpp"

#include <QWidget>
#include <QLabel>

namespace dualcast {

class StreamHealthWidget final : public QWidget {
  Q_OBJECT
public:
  explicit StreamHealthWidget(Platform platform, QWidget *parent = nullptr);
  void setSnapshot(const OutputSnapshot &snapshot);

private:
  Platform platform_;
  QLabel *state_ = nullptr;
  QLabel *metrics_ = nullptr;
};

} // namespace dualcast
