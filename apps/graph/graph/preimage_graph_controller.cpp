#include "preimage_graph_controller.h"
#include "../../shared/poincare_helpers.h"
#include <poincare/serialization_helper.h>
#include <poincare/float.h>

using namespace Escher;

namespace Graph {

PreimageGraphController::PreimageGraphController(
  Responder * parentResponder,
  GraphView * graphView,
  BannerView * bannerView,
  Shared::InteractiveCurveViewRange * curveViewRange,
  Shared::CurveViewCursor * cursor
) :
  CalculationGraphController(
    parentResponder,
    graphView,
    bannerView,
    curveViewRange,
    cursor,
    I18n::Message::NoPreimageFound
  ),
  m_image(NAN)
{
}

Poincare::Coordinate2D<double> PreimageGraphController::computeNewPointOfInterest(double start, double max, Poincare::Context * context, double relativePrecision, double minimalStep, double maximalStep) {
  Poincare::Expression expression = Poincare::Float<double>::Builder(m_image);
  return functionStore()->modelForRecord(m_record)->nextIntersectionFrom(start, max, context, expression, relativePrecision, minimalStep, maximalStep);
}

}
