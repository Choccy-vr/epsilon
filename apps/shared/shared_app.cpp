#include "shared_app.h"
#include "global_context.h"
#include <apps/apps_container.h>

namespace Shared {

void SharedApp::Snapshot::pack(App * app) {
  /* Since the sequence store is now accessible from every app, when exiting
   * any application, we need to tidy it.*/
  AppsContainer::sharedAppsContainer()->globalContext()->tidyDownstreamPoolFrom();
  App::Snapshot::pack(app);
}

}
