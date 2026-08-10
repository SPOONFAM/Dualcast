#pragma once

#include "config/configuration.hpp"

#include <QDialog>

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
  class QLineEdit *serverEdit_ = nullptr;
  class QLineEdit *keyEdit_ = nullptr;
  class QSpinBox *widthEdit_ = nullptr;
  class QSpinBox *heightEdit_ = nullptr;
  class QSpinBox *fpsEdit_ = nullptr;
  class QSpinBox *bitrateEdit_ = nullptr;
  class QSpinBox *audioEdit_ = nullptr;
  class QSpinBox *keyframeEdit_ = nullptr;
  class QComboBox *encoderEdit_ = nullptr;
  class QCheckBox *automaticEdit_ = nullptr;
};

} // namespace dualcast
