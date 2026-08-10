#include "account-dialog.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

namespace dualcast {

AccountDialog::AccountDialog(Platform platform, QWidget *parent) : QDialog(parent)
{
  setWindowTitle(QStringLiteral("%1 account").arg(platformName(platform)));
  auto *layout = new QVBoxLayout(this);
  auto *message = new QLabel(this);
  message->setWordWrap(true);
  if (platform == Platform::TikTok) {
    message->setText(QStringLiteral("TikTok account identity and TikTok LIVE ingest authorization are separate capabilities. Dualcast currently supports official manual LIVE RTMP/RTMPS configuration; it does not scrape, automate a browser, or obtain hidden stream keys."));
  } else {
    message->setText(QStringLiteral("YouTube account authentication uses Google's official OAuth flow. Configure a Google installed-app OAuth client id when building Dualcast, then authorize in your browser."));
  }
  layout->addWidget(message);
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);
}

} // namespace dualcast
