#pragma once

#include "config/configuration.hpp"

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>

namespace dualcast {

class OutputSettingsDialog final : public QDialog {
  Q_OBJECT
public:
  OutputSettingsDialog(Platform platform, DestinationConfig configuration, QWidget *parent = nullptr);
  DestinationConfig configuration() const { return configuration_; }

private slots:
  void acceptAndSave();
  void validateConnection();

private:
  Platform platform_;
  DestinationConfig configuration_;
  QLineEdit *serverEdit_ = nullptr;
  QLineEdit *keyEdit_ = nullptr;
  QSpinBox *widthEdit_ = nullptr;
  QSpinBox *heightEdit_ = nullptr;
  QSpinBox *fpsEdit_ = nullptr;
  QSpinBox *bitrateEdit_ = nullptr;
  QSpinBox *audioEdit_ = nullptr;
  QSpinBox *keyframeEdit_ = nullptr;
  QComboBox *encoderEdit_ = nullptr;
  QCheckBox *automaticEdit_ = nullptr;
};

} // namespace dualcast
