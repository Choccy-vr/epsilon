#ifndef ON_BOARDING_APP_H
#define ON_BOARDING_APP_H

#include "logo_controller.h"
#include "localization_controller.h"
#include "../shared/shared_app.h"

namespace OnBoarding {

class App : public Escher::App {
public:
  class Snapshot : public Shared::SharedApp::Snapshot {
  public:
    App * unpack(Escher::Container * container) override;
    const Descriptor * descriptor() const override;
  };

  int numberOfTimers() override;
  Escher::Timer * timerAtIndex(int i) override;
  void didBecomeActive(Escher::Window * window) override;
  void willBecomeInactive() override;
private:
  App(Snapshot * snapshot);
  void reinitOnBoarding();
  LocalizationController m_localizationController;
  LogoController m_logoController;
};

}

#endif
