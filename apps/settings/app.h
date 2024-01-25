#ifndef SETTINGS_APP_H
#define SETTINGS_APP_H

#include <apps/shared/shared_app.h>

#include "main_controller.h"

namespace Settings {

class App : public Shared::SharedApp {
 public:
  class Descriptor : public Escher::App::Descriptor {
   public:
    I18n::Message name() const override;
    I18n::Message upperName() const override;
    const Escher::Image* icon() const override;
  };
  class Snapshot : public Shared::SharedApp::Snapshot {
   public:
    App* unpack(Escher::Container* container) override;
    const Descriptor* descriptor() const override;
  };
  TELEMETRY_ID("Settings");

 private:
  App(Snapshot* snapshot);
  MainController m_mainController;
  Escher::StackViewController::Default m_stackViewController;
};

}  // namespace Settings

#endif
