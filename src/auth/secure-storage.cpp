#include "secure-storage.hpp"

#include <QByteArray>
#include <QSettings>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

namespace dualcast {
namespace {

constexpr auto kOrganization = "Dualcast";
constexpr auto kApplication = "Dualcast";

void setError(QString *error, const QString &message)
{
  if (error)
    *error = message;
}

#ifdef _WIN32
QByteArray protect(const QByteArray &plain, QString *error)
{
  DATA_BLOB input{static_cast<DWORD>(plain.size()), reinterpret_cast<BYTE *>(const_cast<char *>(plain.constData()))};
  DATA_BLOB output{};
  if (!CryptProtectData(&input, L"Dualcast secret", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) {
    setError(error, QStringLiteral("Windows credential protection failed (%1).").arg(GetLastError()));
    return {};
  }
  QByteArray result(reinterpret_cast<const char *>(output.pbData), static_cast<int>(output.cbData));
  LocalFree(output.pbData);
  return result;
}

QByteArray unprotect(const QByteArray &cipher, QString *error)
{
  DATA_BLOB input{static_cast<DWORD>(cipher.size()), reinterpret_cast<BYTE *>(const_cast<char *>(cipher.constData()))};
  DATA_BLOB output{};
  if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) {
    setError(error, QStringLiteral("Windows credential protection could not decrypt this value."));
    return {};
  }
  QByteArray result(reinterpret_cast<const char *>(output.pbData), static_cast<int>(output.cbData));
  LocalFree(output.pbData);
  return result;
}
#endif

QSettings settings()
{
  return QSettings(QSettings::NativeFormat, QSettings::UserScope, kOrganization, kApplication);
}

} // namespace

bool SecureStorage::store(const QString &id, const QString &secret, QString *error)
{
  if (id.isEmpty() || secret.isEmpty()) {
    setError(error, QStringLiteral("A non-empty secret is required."));
    return false;
  }
#ifdef _WIN32
  const auto encrypted = protect(secret.toUtf8(), error);
  if (encrypted.isEmpty())
    return false;
  auto credentialSettings = settings();
  credentialSettings.setValue(QStringLiteral("secrets/%1").arg(id), encrypted.toBase64());
  credentialSettings.sync();
  return credentialSettings.status() == QSettings::NoError;
#else
  setError(error, QStringLiteral("Secure credential storage is not implemented on this platform yet."));
  return false;
#endif
}

bool SecureStorage::load(const QString &id, QString *secret, QString *error)
{
  if (!secret || id.isEmpty()) {
    setError(error, QStringLiteral("A secret destination is required."));
    return false;
  }
#ifdef _WIN32
  const auto value = settings().value(QStringLiteral("secrets/%1").arg(id)).toByteArray();
  if (value.isEmpty()) {
    setError(error, QStringLiteral("No saved credential exists."));
    return false;
  }
  const auto decrypted = unprotect(QByteArray::fromBase64(value), error);
  if (decrypted.isEmpty())
    return false;
  *secret = QString::fromUtf8(decrypted);
  return true;
#else
  setError(error, QStringLiteral("Secure credential storage is not implemented on this platform yet."));
  return false;
#endif
}

bool SecureStorage::remove(const QString &id, QString *error)
{
  if (id.isEmpty()) {
    setError(error, QStringLiteral("A secret identifier is required."));
    return false;
  }
#ifdef _WIN32
  auto credentialSettings = settings();
  credentialSettings.remove(QStringLiteral("secrets/%1").arg(id));
  credentialSettings.sync();
  return credentialSettings.status() == QSettings::NoError;
#else
  setError(error, QStringLiteral("Secure credential storage is not implemented on this platform yet."));
  return false;
#endif
}

} // namespace dualcast
