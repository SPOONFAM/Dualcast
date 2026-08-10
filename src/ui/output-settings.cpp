#include "output-settings.hpp"

#include "auth/secure-storage.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <utility>

namespace dualcast {

OutputSettingsDialog::OutputSettingsDialog(Platform platform, DestinationConfig configuration, QWidget *parent)
    : QDialog(parent), platform_(platform), configuration_(std::move(configuration))
{
  setWindowTitle(QStringLiteral("%1 output settings").arg(platformName(platform)));
  setModal(true);
  auto *root = new QVBoxLayout(this);
  auto *destinationBox = new QGroupBox(QStringLiteral("RTMP / RTMPS ingest"), this);
  auto *form = new QFormLayout(destinationBox);
  serverEdit_ = new QLineEdit(configuration_.server, destinationBox);
  serverEdit_->setPlaceholderText(QStringLiteral("rtmps://..."));
  keyEdit_ = new QLineEdit(destinationBox);
  keyEdit_->setEchoMode(QLineEdit::Password);
  keyEdit_->setPlaceholderText(QStringLiteral("Leave blank to keep the saved key"));
  form->addRow(QStringLiteral("Server:"), serverEdit_);
  form->addRow(QStringLiteral("Stream key:"), keyEdit_);
  root->addWidget(destinationBox);

  auto *qualityBox = new QGroupBox(QStringLiteral("Video and audio"), this);
  auto *quality = new QFormLayout(qualityBox);
  widthEdit_ = new QSpinBox(qualityBox); widthEdit_->setRange(160, 7680); widthEdit_->setValue(configuration_.width);
  heightEdit_ = new QSpinBox(qualityBox); heightEdit_->setRange(160, 7680); heightEdit_->setValue(configuration_.height);
  fpsEdit_ = new QSpinBox(qualityBox); fpsEdit_->setRange(1, 120); fpsEdit_->setValue(configuration_.fps);
  bitrateEdit_ = new QSpinBox(qualityBox); bitrateEdit_->setRange(100, 100000); bitrateEdit_->setValue(configuration_.bitrateKbps); bitrateEdit_->setSuffix(QStringLiteral(" kbps"));
  audioEdit_ = new QSpinBox(qualityBox); audioEdit_->setRange(32, 512); audioEdit_->setValue(configuration_.audioBitrateKbps); audioEdit_->setSuffix(QStringLiteral(" kbps"));
  keyframeEdit_ = new QSpinBox(qualityBox); keyframeEdit_->setRange(1, 10); keyframeEdit_->setValue(configuration_.keyframeIntervalSec); keyframeEdit_->setSuffix(QStringLiteral(" s"));
  encoderEdit_ = new QComboBox(qualityBox);
  encoderEdit_->addItem(QStringLiteral("Automatic"), static_cast<int>(EncoderPreference::Automatic));
  encoderEdit_->addItem(QStringLiteral("NVIDIA NVENC"), static_cast<int>(EncoderPreference::Nvenc));
  encoderEdit_->addItem(QStringLiteral("AMD AMF"), static_cast<int>(EncoderPreference::Amf));
  encoderEdit_->addItem(QStringLiteral("Intel Quick Sync"), static_cast<int>(EncoderPreference::Qsv));
  encoderEdit_->addItem(QStringLiteral("x264"), static_cast<int>(EncoderPreference::X264));
  encoderEdit_->setCurrentIndex(encoderEdit_->findData(static_cast<int>(configuration_.encoder)));
  automaticEdit_ = new QCheckBox(QStringLiteral("Use recommended quality defaults"), qualityBox); automaticEdit_->setChecked(configuration_.automaticQuality);
  quality->addRow(QStringLiteral("Width:"), widthEdit_);
  quality->addRow(QStringLiteral("Height:"), heightEdit_);
  quality->addRow(QStringLiteral("FPS:"), fpsEdit_);
  quality->addRow(QStringLiteral("Video bitrate:"), bitrateEdit_);
  quality->addRow(QStringLiteral("Audio bitrate:"), audioEdit_);
  quality->addRow(QStringLiteral("Keyframe interval:"), keyframeEdit_);
  quality->addRow(QStringLiteral("Encoder:"), encoderEdit_);
  quality->addRow(automaticEdit_);
  root->addWidget(qualityBox);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
  auto *testButton = buttons->addButton(QStringLiteral("Validate settings"), QDialogButtonBox::ActionRole);
  connect(testButton, &QPushButton::clicked, this, &OutputSettingsDialog::validateConnection);
  connect(buttons, &QDialogButtonBox::accepted, this, &OutputSettingsDialog::acceptAndSave);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  root->addWidget(buttons);
}

void OutputSettingsDialog::validateConnection()
{
  const auto server = serverEdit_->text().trimmed();
  if (!server.startsWith(QStringLiteral("rtmp://")) && !server.startsWith(QStringLiteral("rtmps://"))) {
    QMessageBox::warning(this, QStringLiteral("Invalid server"), QStringLiteral("Use an RTMP or RTMPS ingest URL."));
    return;
  }
  QMessageBox::information(this, QStringLiteral("Settings look valid"), QStringLiteral("The URL format is valid. OBS will verify ingest authorization when the destination starts; no network probe was performed."));
}

void OutputSettingsDialog::acceptAndSave()
{
  const auto server = serverEdit_->text().trimmed();
  if (!server.startsWith(QStringLiteral("rtmp://")) && !server.startsWith(QStringLiteral("rtmps://"))) {
    QMessageBox::warning(this, QStringLiteral("Invalid server"), QStringLiteral("Use an RTMP or RTMPS ingest URL."));
    return;
  }
  configuration_.server = server;
  configuration_.width = widthEdit_->value();
  configuration_.height = heightEdit_->value();
  configuration_.fps = fpsEdit_->value();
  configuration_.bitrateKbps = bitrateEdit_->value();
  configuration_.audioBitrateKbps = audioEdit_->value();
  configuration_.keyframeIntervalSec = keyframeEdit_->value();
  configuration_.encoder = static_cast<EncoderPreference>(encoderEdit_->currentData().toInt());
  configuration_.automaticQuality = automaticEdit_->isChecked();
  configuration_.configured = true;
  if (!keyEdit_->text().isEmpty()) {
    QString error;
    configuration_.secretId = ConfigurationStore::secretId(platform_);
    if (!SecureStorage::store(configuration_.secretId, keyEdit_->text(), &error)) {
      QMessageBox::critical(this, QStringLiteral("Could not save stream key"), error);
      return;
    }
  } else {
    configuration_.secretId = configuration_.secretId.isEmpty() ? ConfigurationStore::secretId(platform_) : configuration_.secretId;
    QString existingSecret;
    QString storageError;
    if (!SecureStorage::load(configuration_.secretId, &existingSecret, &storageError)) {
      QMessageBox::warning(this, QStringLiteral("Stream key required"), QStringLiteral("Enter a stream key and save it securely before configuring this destination."));
      return;
    }
  }
  accept();
}

} // namespace dualcast
