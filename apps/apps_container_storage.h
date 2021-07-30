#ifndef APPS_CONTAINER_STORAGE_H
#define APPS_CONTAINER_STORAGE_H

#include "apps_container.h"

#ifndef APPS_CONTAINER_SNAPSHOT_DECLARATIONS
#error Missing snapshot declarations
#endif

class AppsContainerStorage : public AppsContainer {
public:
  AppsContainerStorage();
  int numberOfApps() override;
  Escher::App::Snapshot * appSnapshotAtIndex(int index) override;
  void * currentAppBuffer() override;
  union Apps {
  public:
    /* Enforce a trivial constructor and destructor that just leave the memory
     * unmodified. This way, m_apps can be trivially destructed. */
    Apps() {};
    ~Apps() {};
  private:
    APPS_CONTAINER_APPS_DECLARATION
    Home::App m_homeApp;
    OnBoarding::App m_onBoardingApp;
    HardwareTest::App m_hardwareTestApp;
    USB::App m_usbApp;
  };
private:
  APPS_CONTAINER_SNAPSHOT_DECLARATIONS
};

#endif
