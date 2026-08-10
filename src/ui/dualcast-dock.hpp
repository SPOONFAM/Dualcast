#pragma once

#include "config/configuration.hpp"

#include <QWidget>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>

#include <memory>

namespace dualcast {
class OutputManager;
class OAuthManager;
class YouTubeProvider;
class TikTokProvider;
class StreamHealthWidget;

class DualcastDock final : public QWidget {
  Q_OBJECT
public:
  explicit DualcastDock(QWidget *parent = nullptr);
  ~DualcastDock() override;
  bool startsWithObs() const { return configuration_.startWithObs; }

private slots:
  void openYouTubeSettings();
  void openTikTokSettings();
  void connectYouTube();
  void connectTikTokInfo();
  void startAll();
  void stopAll();
  void startYouTube();
  void startTikTok();
  void refresh();

private:
  DualcastConfiguration configuration_;
  std::unique_ptr<OAuthManager> oauth_;
  std::unique_ptr<YouTubeProvider> youtube_;
  std::unique_ptr<TikTokProvider> tiktok_;
  std::unique_ptr<OutputManager> outputs_;
  QLabel *youtubeStatus_ = nullptr;
  QLabel *tiktokStatus_ = nullptr;
  QLabel *bandwidth_ = nullptr;
  QLabel *encoderPlan_ = nullptr;
  QCheckBox *youtubeEnabled_ = nullptr;
  QCheckBox *tiktokEnabled_ = nullptr;
  QPushButton *startAllButton_ = nullptr;
  QPushButton *stopAllButton_ = nullptr;
  QPushButton *youtubeStartButton_ = nullptr;
  QPushButton *tiktokStartButton_ = nullptr;
  std::unique_ptr<StreamHealthWidget> youtubeHealth_;
  std::unique_ptr<StreamHealthWidget> tiktokHealth_;

  QWidget *makeDestinationRow(Platform platform);
  void openSettings(Platform platform);
  void updateDestinationLabels();
};

} // namespace dualcast
