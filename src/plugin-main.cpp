#include <obs-frontend-api.h>
#include <obs-module.h>

#include "ui/dualcast-dock.hpp"

#include <QMetaObject>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(Dualcast, "en-US")

namespace {
constexpr const char *kDockId = "dualcast_dock";
dualcast::DualcastDock *g_dock = nullptr;

void onFrontendEvent(enum obs_frontend_event event, void *)
{
  if (event == OBS_FRONTEND_EVENT_STREAMING_STARTING && g_dock && g_dock->startsWithObs())
    QMetaObject::invokeMethod(g_dock, "startAll", Qt::QueuedConnection);
}
} // namespace

bool obs_module_load(void)
{
  g_dock = new dualcast::DualcastDock();
  if (!obs_frontend_add_dock_by_id(kDockId, "Dualcast", g_dock)) {
    delete g_dock;
    g_dock = nullptr;
    obs_log(LOG_ERROR, "[Dualcast] Could not register the Dualcast dock.");
    return false;
  }
  obs_frontend_add_event_callback(onFrontendEvent, nullptr);
  obs_log(LOG_INFO, "[Dualcast] loaded");
  return true;
}

void obs_module_unload(void)
{
  obs_frontend_remove_event_callback(onFrontendEvent, nullptr);
  obs_frontend_remove_dock(kDockId);
  g_dock = nullptr;
  obs_log(LOG_INFO, "[Dualcast] unloaded");
}
