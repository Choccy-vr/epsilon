#include "apps_container_storage.h"

#ifndef APPS_CONTAINER_SNAPSHOT_CONSTRUCTORS
#error Missing snapshot constructors
#endif

#ifndef APPS_CONTAINER_SNAPSHOT_LIST
#error Missing snapshot list
#endif

#ifndef APPS_CONTAINER_SNAPSHOT_COUNT
#error Missing snapshot count
#endif

constexpr int k_numberOfCommonApps = 1+APPS_CONTAINER_SNAPSHOT_COUNT; // Take the Home app into account

constexpr size_t k_appBufferSize = 64*1024;
static_assert(k_appBufferSize >= sizeof(AppsContainerStorage::Apps),"The RAM allocated to App execution is not big enough to hold all official apps (the union size overflows)");

char __attribute__((section(".app_buffer"))) s_appBuffer[k_appBufferSize];

void * AppsContainerStorage::currentAppBuffer() {
  return s_appBuffer;
}

AppsContainerStorage::AppsContainerStorage() :
  AppsContainer()
  APPS_CONTAINER_SNAPSHOT_CONSTRUCTORS
{
}

int AppsContainerStorage::numberOfApps() {
  return k_numberOfCommonApps;
}

Escher::App::Snapshot * AppsContainerStorage::appSnapshotAtIndex(int index) {
  if (index < 0) {
    return nullptr;
  }
  Escher::App::Snapshot * snapshots[] = {
    homeAppSnapshot()
    APPS_CONTAINER_SNAPSHOT_LIST
  };
  assert(sizeof(snapshots)/sizeof(snapshots[0]) == k_numberOfCommonApps);
  assert(index >= 0 && index < k_numberOfCommonApps);
  return snapshots[index];
}
