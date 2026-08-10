#pragma once

#include <QDialog>

namespace dualcast {

class LayoutEditor final : public QDialog {
  Q_OBJECT
public:
  explicit LayoutEditor(QWidget *parent = nullptr);
};

} // namespace dualcast
