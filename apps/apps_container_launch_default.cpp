#include "apps_container.h"

Escher::App::Snapshot * AppsContainer::initialAppSnapshot() {
  // The backlight has not been initialized
  Ion::Backlight::init();
  return appSnapshotAtIndex(numberOfBuiltinApps() == 2 ? 1 : 0);
}
