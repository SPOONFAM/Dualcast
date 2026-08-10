#pragma once

#include <QString>

namespace dualcast {

class SecureStorage final {
public:
  static bool store(const QString &id, const QString &secret, QString *error = nullptr);
  static bool load(const QString &id, QString *secret, QString *error = nullptr);
  static bool remove(const QString &id, QString *error = nullptr);
};

} // namespace dualcast
