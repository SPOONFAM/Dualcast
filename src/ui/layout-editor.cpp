#include "layout-editor.hpp"

#include <obs-frontend-api.h>
#include <obs.h>

#include <QDialogButtonBox>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QListWidget>
#include <QSplitter>
#include <QVBoxLayout>

namespace dualcast {

LayoutEditor::LayoutEditor(QWidget *parent) : QDialog(parent)
{
  setWindowTitle(QStringLiteral("TikTok layout editor"));
  resize(760, 600);
  auto *root = new QVBoxLayout(this);
  auto *notice = new QLabel(QStringLiteral("Layout editing is stored as a source-placement model. The vertical compositor is intentionally gated until it can render existing OBS sources through a supported output path."), this);
  notice->setWordWrap(true);
  root->addWidget(notice);
  auto *splitter = new QSplitter(this);
  auto *sources = new QListWidget(splitter);
  auto *scene = new QGraphicsScene(splitter);
  scene->setSceneRect(0, 0, 360, 640);
  scene->addRect(0, 0, 360, 640, QPen(Qt::gray), QBrush(QColor(30, 30, 30)));
  auto *view = new QGraphicsView(scene, splitter);
  view->setFixedSize(400, 680);
  auto *currentScenes = obs_frontend_get_scenes();
  if (currentScenes) {
    for (size_t i = 0; currentScenes[i]; ++i) {
      const char *name = obs_source_get_name(currentScenes[i]);
      if (name) sources->addItem(QString::fromUtf8(name));
      obs_source_release(currentScenes[i]);
    }
    bfree(currentScenes);
  }
  splitter->addWidget(sources);
  splitter->addWidget(view);
  root->addWidget(splitter, 1);
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  root->addWidget(buttons);
}

} // namespace dualcast
