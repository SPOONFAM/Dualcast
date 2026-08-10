#include "dualcast-dock.hpp"

#include "auth/oauth-manager.hpp"
#include "core/bandwidth-monitor.hpp"
#include "core/output-manager.hpp"
#include "providers/tiktok-provider.hpp"
#include "providers/youtube-provider.hpp"
#include "ui/account-dialog.hpp"
#include "ui/output-settings.hpp"
#include "ui/stream-health-widget.hpp"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

namespace dualcast {

DualcastDock::DualcastDock(QWidget *parent) : QWidget(parent), configuration_(ConfigurationStore::load())
{
  setObjectName(QStringLiteral("DualcastDock"));
  setMinimumWidth(300);
  oauth_ = std::make_unique<OAuthManager>(this);
  youtube_ = std::make_unique<YouTubeProvider>(oauth_.get());
  tiktok_ = std::make_unique<TikTokProvider>();
  outputs_ = std::make_unique<OutputManager>(this);

  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(10, 10, 10, 10);
  auto *title = new QLabel(QStringLiteral("Dualcast"), this);
  title->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: bold;"));
  root->addWidget(title);
  auto *subtitle = new QLabel(QStringLiteral("Stream from this OBS instance to independent destinations."), this);
  subtitle->setWordWrap(true);
  root->addWidget(subtitle);

  root->addWidget(makeDestinationRow(Platform::YouTube));
  root->addWidget(makeDestinationRow(Platform::TikTok));

  auto *outputsBox = new QGroupBox(QStringLiteral("OUTPUTS"), this);
  auto *outputsLayout = new QVBoxLayout(outputsBox);
  youtubeEnabled_ = new QCheckBox(QStringLiteral("YouTube"), outputsBox); youtubeEnabled_->setChecked(configuration_.youtube.enabled);
  tiktokEnabled_ = new QCheckBox(QStringLiteral("TikTok"), outputsBox); tiktokEnabled_->setChecked(configuration_.tiktok.enabled);
  outputsLayout->addWidget(youtubeEnabled_);
  outputsLayout->addWidget(tiktokEnabled_);
  auto *startWithObs = new QCheckBox(QStringLiteral("Start enabled Dualcast destinations when OBS starts streaming"), outputsBox);
  startWithObs->setChecked(configuration_.startWithObs);
  outputsLayout->addWidget(startWithObs);
  root->addWidget(outputsBox);

  startAllButton_ = new QPushButton(QStringLiteral("GO LIVE EVERYWHERE"), this);
  startAllButton_->setMinimumHeight(34);
  stopAllButton_ = new QPushButton(QStringLiteral("STOP ALL STREAMS"), this);
  root->addWidget(startAllButton_);
  root->addWidget(stopAllButton_);

  auto *healthBox = new QGroupBox(QStringLiteral("STREAM HEALTH"), this);
  auto *healthLayout = new QVBoxLayout(healthBox);
  youtubeHealth_ = std::make_unique<StreamHealthWidget>(Platform::YouTube, healthBox);
  tiktokHealth_ = std::make_unique<StreamHealthWidget>(Platform::TikTok, healthBox);
  healthLayout->addWidget(youtubeHealth_.get());
  healthLayout->addWidget(tiktokHealth_.get());
  root->addWidget(healthBox);

  bandwidth_ = new QLabel(this); bandwidth_->setWordWrap(true); root->addWidget(bandwidth_);
  encoderPlan_ = new QLabel(this); encoderPlan_->setWordWrap(true); root->addWidget(encoderPlan_);
  root->addStretch(1);

  connect(youtubeEnabled_, &QCheckBox::toggled, this, [this](bool enabled) { configuration_.youtube.enabled = enabled; ConfigurationStore::save(configuration_); refresh(); });
  connect(tiktokEnabled_, &QCheckBox::toggled, this, [this](bool enabled) { configuration_.tiktok.enabled = enabled; ConfigurationStore::save(configuration_); refresh(); });
  connect(startWithObs, &QCheckBox::toggled, this, [this](bool enabled) { configuration_.startWithObs = enabled; ConfigurationStore::save(configuration_); });
  connect(startAllButton_, &QPushButton::clicked, this, &DualcastDock::startAll);
  connect(stopAllButton_, &QPushButton::clicked, this, &DualcastDock::stopAll);
  connect(outputs_.get(), &OutputManager::statusChanged, this, &DualcastDock::refresh);
  connect(outputs_.get(), &OutputManager::platformStopped, this, [this](Platform platform, int code) {
    if (code != OBS_OUTPUT_SUCCESS && code != OBS_OUTPUT_DISCONNECTED)
      QMessageBox::warning(this, QStringLiteral("%1 stopped").arg(platformName(platform)), QStringLiteral("The destination stopped with OBS output code %1. The other destination was left running.").arg(code));
    refresh();
  });
  refresh();
  if (configuration_.firstRun) {
    QTimer::singleShot(0, this, [this] {
      QMessageBox::information(this, QStringLiteral("Welcome to Dualcast"), QStringLiteral("Stream from one OBS setup to multiple destinations. Connect YouTube with official OAuth, or configure TikTok LIVE and YouTube manually with official RTMP/RTMPS ingest details.\n\nDualcast never asks for passwords or retrieves hidden stream keys."));
      configuration_.firstRun = false;
      ConfigurationStore::save(configuration_);
    });
  }
}

DualcastDock::~DualcastDock()
{
  ConfigurationStore::save(configuration_);
}

QWidget *DualcastDock::makeDestinationRow(Platform platform)
{
  auto *box = new QGroupBox(platformName(platform), this);
  auto *layout = new QVBoxLayout(box);
  auto *status = new QLabel(box);
  auto *account = new QLabel(box);
  auto *buttons = new QHBoxLayout();
  auto *settings = new QPushButton(QStringLiteral("Settings"), box);
  auto *connectButton = new QPushButton(platform == Platform::YouTube ? QStringLiteral("Connect YouTube") : QStringLiteral("TikTok account info"), box);
  auto *start = new QPushButton(QStringLiteral("Start"), box);
  buttons->addWidget(connectButton);
  buttons->addWidget(settings);
  buttons->addWidget(start);
  layout->addWidget(status);
  const auto &destinationConfig = destination(configuration_, platform);
  account->setText(destinationConfig.accountName.isEmpty() ? (destinationConfig.configured ? QStringLiteral("Manual RTMP/RTMPS ingest") : QString{}) : destinationConfig.accountName);
  layout->addWidget(account);
  layout->addLayout(buttons);
  if (platform == Platform::YouTube) {
    youtubeStatus_ = status;
    youtubeStartButton_ = start;
    connectButton->setObjectName(QStringLiteral("connectYouTube"));
    QObject::connect(connectButton, &QPushButton::clicked, this, &DualcastDock::connectYouTube);
    connect(settings, &QPushButton::clicked, this, &DualcastDock::openYouTubeSettings);
    QObject::connect(start, &QPushButton::clicked, this, [this] { if (outputs_->isActive(Platform::YouTube)) outputs_->stop(Platform::YouTube); else startYouTube(); });
  } else {
    tiktokStatus_ = status;
    tiktokStartButton_ = start;
    QObject::connect(connectButton, &QPushButton::clicked, this, &DualcastDock::connectTikTokInfo);
    connect(settings, &QPushButton::clicked, this, &DualcastDock::openTikTokSettings);
    QObject::connect(start, &QPushButton::clicked, this, [this] { if (outputs_->isActive(Platform::TikTok)) outputs_->stop(Platform::TikTok); else startTikTok(); });
  }
  return box;
}

void DualcastDock::openSettings(Platform platform)
{
  auto &current = destination(configuration_, platform);
  OutputSettingsDialog dialog(platform, current, this);
  if (dialog.exec() == QDialog::Accepted) {
    current = dialog.configuration();
    ConfigurationStore::save(configuration_);
    refresh();
  }
}

void DualcastDock::openYouTubeSettings() { openSettings(Platform::YouTube); }
void DualcastDock::openTikTokSettings() { openSettings(Platform::TikTok); }

void DualcastDock::connectYouTube()
{
  QString error;
  if (!youtube_->authenticate(&error)) {
    QMessageBox::warning(this, QStringLiteral("YouTube connection"), error);
    return;
  }
  configuration_.youtube.oauthConnected = true;
  configuration_.youtube.accountName = youtube_->account().displayName;
  ConfigurationStore::save(configuration_);
  refresh();
}

void DualcastDock::connectTikTokInfo()
{
  AccountDialog(Platform::TikTok, this).exec();
}

void DualcastDock::startAll()
{
  QString error;
  outputs_->startAll(configuration_, &error);
  if (!error.isEmpty())
    QMessageBox::warning(this, QStringLiteral("Dualcast preflight"), error);
  refresh();
}

void DualcastDock::stopAll() { outputs_->stopAll(); refresh(); }

void DualcastDock::startYouTube()
{
  QString error;
  if (!outputs_->start(Platform::YouTube, configuration_, &error))
    QMessageBox::warning(this, QStringLiteral("YouTube"), error);
  refresh();
}

void DualcastDock::startTikTok()
{
  QString error;
  if (!outputs_->start(Platform::TikTok, configuration_, &error))
    QMessageBox::warning(this, QStringLiteral("TikTok"), error);
  refresh();
}

void DualcastDock::updateDestinationLabels()
{
  const auto youtube = outputs_->snapshot(Platform::YouTube);
  const auto tiktok = outputs_->snapshot(Platform::TikTok);
  youtubeStatus_->setText(outputs_->isActive(Platform::YouTube) ? QStringLiteral("[ LIVE ]") : configuration_.youtube.configured ? QStringLiteral("[ Configured ]") : QStringLiteral("[ Not configured ]"));
  tiktokStatus_->setText(outputs_->isActive(Platform::TikTok) ? QStringLiteral("[ LIVE ]") : configuration_.tiktok.configured ? QStringLiteral("[ Configured ]") : QStringLiteral("[ Not configured ]"));
  youtubeStartButton_->setText(outputs_->isActive(Platform::YouTube) ? QStringLiteral("Stop") : QStringLiteral("Start"));
  tiktokStartButton_->setText(outputs_->isActive(Platform::TikTok) ? QStringLiteral("Stop") : QStringLiteral("Start"));
  youtubeHealth_->setSnapshot(youtube);
  tiktokHealth_->setSnapshot(tiktok);
}

void DualcastDock::refresh()
{
  updateDestinationLabels();
  const auto estimate = BandwidthMonitor::estimate(configuration_);
  bandwidth_->setText(QStringLiteral("Estimated upload: %1 Mbps (video %2 kbps + audio %3 kbps + overhead %4 kbps)%5")
                          .arg(QString::number(estimate.totalKbps / 1000.0, 'f', 1))
                          .arg(estimate.videoKbps)
                          .arg(estimate.audioKbps)
                          .arg(estimate.overheadKbps)
                          .arg(estimate.warning.isEmpty() ? QString{} : QStringLiteral("\nWarning: %1").arg(estimate.warning)));
  encoderPlan_->setText(QStringLiteral("Encoder usage: %1").arg(outputs_->encoderPlan(configuration_)));
}

} // namespace dualcast
