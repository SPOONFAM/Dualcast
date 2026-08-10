#include "stream-health-widget.hpp"

#include <QLabel>
#include <QVBoxLayout>

namespace dualcast {

StreamHealthWidget::StreamHealthWidget(Platform platform, QWidget *parent) : QWidget(parent), platform_(platform)
{
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  auto *title = new QLabel(platformName(platform).toUpper(), this);
  title->setStyleSheet(QStringLiteral("font-weight: bold;"));
  state_ = new QLabel(QStringLiteral("[ Offline ]"), this);
  metrics_ = new QLabel(QStringLiteral("No output running"), this);
  metrics_->setWordWrap(true);
  layout->addWidget(title);
  layout->addWidget(state_);
  layout->addWidget(metrics_);
}

void StreamHealthWidget::setSnapshot(const OutputSnapshot &snapshot)
{
  if (!snapshot.active) {
    state_->setText(snapshot.reconnecting ? QStringLiteral("[ Reconnecting ]") : QStringLiteral("[ Offline ]"));
    metrics_->setText(snapshot.lastError.isEmpty() ? QStringLiteral("No output running") : snapshot.lastError);
    return;
  }
  const auto quality = snapshot.congestion < 0.25f ? QStringLiteral("Excellent") : snapshot.congestion < 0.6f ? QStringLiteral("Fair") : QStringLiteral("Congested");
  state_->setText(QStringLiteral("[ %1 ]").arg(quality));
  metrics_->setText(QStringLiteral("%1 FPS\n%2 kbps\n%3 dropped frames\nUptime %4:%5")
                        .arg(snapshot.totalFrames > 0 && snapshot.uptimeSec > 0 ? QString::number(snapshot.totalFrames / snapshot.uptimeSec) : QStringLiteral("-"))
                        .arg(snapshot.bitrateKbps)
                        .arg(snapshot.droppedFrames)
                        .arg(snapshot.uptimeSec / 60, 2, 10, QChar('0'))
                        .arg(snapshot.uptimeSec % 60, 2, 10, QChar('0')));
}

} // namespace dualcast
