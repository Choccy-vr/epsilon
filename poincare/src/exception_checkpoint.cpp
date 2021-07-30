#include <poincare/exception_checkpoint.h>

namespace Poincare {

ExceptionCheckpoint * ExceptionCheckpoint::s_topmostExceptionCheckpoint;

ExceptionCheckpoint::ExceptionCheckpoint() :
  Checkpoint(),
  m_parent(s_topmostExceptionCheckpoint)
{
}

ExceptionCheckpoint::~ExceptionCheckpoint() {
  s_topmostExceptionCheckpoint = m_parent;
}

bool ExceptionCheckpoint::setActive(bool interruption) {
  if (!interruption) {
    s_topmostExceptionCheckpoint = this;
  }
  return !interruption;
}

void ExceptionCheckpoint::rollback() {
  Checkpoint::rollback();
  longjmp(m_jumpBuffer, 1);
}

}
