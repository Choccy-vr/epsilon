#include "app.h"
#include <apps/i18n.h>
#include "../apps_container.h"
#include <assert.h>

namespace USB {

I18n::Message App::Descriptor::name() const {
  return upperName();
}

I18n::Message App::Descriptor::upperName() const {
  return I18n::Message::USBAppCapital;
}

App * App::Snapshot::unpack(Escher::Container * container) {
  return new (container->currentAppBuffer()) App(this);
}

static constexpr App::Descriptor sDescriptor;

const App::Descriptor * App::Snapshot::descriptor() const {
  return &sDescriptor;
}

App::App(Snapshot * snapshot) :
  Escher::App(snapshot, &m_usbConnectedController)
{
}

bool App::processEvent(Ion::Events::Event e) {
  return false;
}

}
