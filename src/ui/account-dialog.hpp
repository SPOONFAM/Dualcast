#pragma once

#include "config/configuration.hpp"

#include <QDialog>

namespace dualcast {

class AccountDialog final : public QDialog {
  Q_OBJECT
public:
  explicit AccountDialog(Platform platform, QWidget *parent = nullptr);
};

} // namespace dualcast
