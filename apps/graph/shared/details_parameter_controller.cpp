#include "details_parameter_controller.h"

#include <apps/i18n.h>
#include <apps/shared/poincare_helpers.h>
#include <assert.h>
#include <poincare/expression.h>
#include <poincare/layout_helper.h>

#include "../app.h"

using namespace Escher;
using namespace Poincare;
namespace Graph {

DetailsParameterController::DetailsParameterController(
    Responder *parentResponder)
    : SelectableListViewController(parentResponder, this) {}

bool DetailsParameterController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    return true;
  }
  if (event == Ion::Events::Left) {
    stackController()->pop();
    return true;
  }
  return false;
}

const char *DetailsParameterController::title() {
  return I18n::translate(I18n::Message::Details);
}

void DetailsParameterController::viewWillAppear() {
  ViewController::viewWillAppear();
  assert(!m_record.isNull());
  selectRow(0);
  m_selectableListView.reloadData();
}

KDCoordinate DetailsParameterController::nonMemoizedRowHeight(int row) {
  DetailCell tempCell;
  return protectedNonMemoizedRowHeight(&tempCell, row);
}

void DetailsParameterController::fillCellForRow(HighlightCell *cell, int row) {
  assert(0 <= row && row < k_numberOfDataPoints);
  DetailCell *myCell = static_cast<DetailCell *>(cell);
  if (row == k_indexOfCurveTypeRow) {
    myCell->label()->setMessage(I18n::Message::Type);
    myCell->subLabel()->setMessage(I18n::Message::Default);
    myCell->accessory()->setText(
        I18n::translate(function()->properties().caption()));
  } else {
    myCell->label()->setMessage(detailsTitle(row - 1));
    double value = detailsValue(row - 1);
    if (row - 1 == 0 && functionIsNonVerticalLine()) {
      assert(std::isnan(value));
      /* For the line's equation cell, we want the detail description (y=mx+b)
       * to be displayed as the value would : a large font accessory. */
      myCell->accessory()->setText(
          I18n::translate(detailsDescription(row - 1)));
      myCell->subLabel()->setMessage(I18n::Message::Default);
    } else {
      constexpr int precision =
          Poincare::Preferences::VeryLargeNumberOfSignificantDigits;
      constexpr int bufferSize =
          Poincare::PrintFloat::charSizeForFloatsWithPrecision(precision);
      char buffer[bufferSize];
      Shared::PoincareHelpers::ConvertFloatToTextWithDisplayMode<double>(
          value, buffer, bufferSize, precision,
          Poincare::Preferences::PrintFloatMode::Decimal);
      myCell->accessory()->setText(buffer);
      myCell->subLabel()->setMessage(detailsDescription(row - 1));
    }
  }
}

DetailCell *DetailsParameterController::reusableCell(int index, int type) {
  assert(0 <= index && index < reusableCellCount(type));
  return &m_cells[index];
}

void DetailsParameterController::setRecord(Ion::Storage::Record record) {
  m_record = record;
  if (!m_record.isNull()) {
    Shared::ExpiringPointer<Shared::ContinuousFunction> f = function();
    Poincare::Context *context = App::app()->localContext();
    if (functionIsNonVerticalLine()) {
      double slope, intercept;
      f->getLineParameters(&slope, &intercept, context);
      setLineDetailsValues(slope, intercept);
    } else if (f->properties().isConic() && f->properties().isCartesian()) {
      /* For now this is only implemented for cartesian conics but could also
       * be for polar and parametric conics. */
      CartesianConic c = f->cartesianConicParameters(context);
      setConicDetailsValues(&c);
    }
  }
}

int DetailsParameterController::detailsNumberOfSections() const {
  if (m_record.isNull() || function()->properties().hideDetails()) {
    return 0;
  }
  ExamMode examMode = Preferences::SharedPreferences()->examMode();
  if (examMode.forbidGraphDetails()) {
    return 0;
  }
  if (functionIsNonVerticalLine()) {
    return k_lineDetailsSections;
  }
  if (examMode.forbidImplicitPlots()) {
    // No conic details are expected if implicit plots are disabled
    return 0;
  }
  Shared::ContinuousFunctionProperties properties = function()->properties();
  if (!properties.isConic() || !properties.isCartesian()) {
    /* For now this is only implemented for cartesian conics but could also
     * be for polar and parametric conics. */
    return 0;
  }

  switch (properties.conicShape()) {
    case Conic::Shape::Circle:
      return k_circleDetailsSections;
    case Conic::Shape::Ellipse:
      return k_ellipseDetailsSections;
    case Conic::Shape::Parabola:
      return k_parabolaDetailsSections;
    default:
      assert(properties.conicShape() == Conic::Shape::Hyperbola);
      return k_hyperbolaDetailsSections;
  }
}

Shared::ExpiringPointer<Shared::ContinuousFunction>
DetailsParameterController::function() const {
  assert(!m_record.isNull());
  App *myApp = App::app();
  return myApp->functionStore()->modelForRecord(m_record);
}

I18n::Message DetailsParameterController::detailsTitle(int i) const {
  assert(i < detailsNumberOfSections());

  if (functionIsNonVerticalLine()) {
    constexpr I18n::Message k_titles[k_lineDetailsSections] = {
        I18n::Message::Equation,
        I18n::Message::M,
        I18n::Message::LineYInterceptTitle,
    };
    return k_titles[i];
  }

  switch (function()->properties().conicShape()) {
    case Conic::Shape::Circle: {
      constexpr I18n::Message k_titles[k_circleDetailsSections] = {
          I18n::Message::R,
          I18n::Message::CenterAbscissaTitle,
          I18n::Message::CenterOrdinateTitle,
      };
      return k_titles[i];
    }
    case Conic::Shape::Ellipse: {
      constexpr I18n::Message k_titles[k_ellipseDetailsSections] = {
          I18n::Message::A,
          I18n::Message::B,
          I18n::Message::C,
          I18n::Message::E,
          I18n::Message::CenterAbscissaTitle,
          I18n::Message::CenterOrdinateTitle,
      };
      return k_titles[i];
    }
    case Conic::Shape::Parabola: {
      constexpr I18n::Message k_titles[k_parabolaDetailsSections] = {
          I18n::Message::P,
          I18n::Message::ParabolaVertexAbscissaTitle,
          I18n::Message::ParabolaVertexOrdinateTitle,
      };
      return k_titles[i];
    }
    default: {
      assert(function()->properties().conicShape() == Conic::Shape::Hyperbola);
      constexpr I18n::Message k_titles[k_hyperbolaDetailsSections] = {
          I18n::Message::A,
          I18n::Message::B,
          I18n::Message::C,
          I18n::Message::E,
          I18n::Message::CenterAbscissaTitle,
          I18n::Message::CenterOrdinateTitle,
      };
      return k_titles[i];
    }
  }
}

I18n::Message DetailsParameterController::detailsDescription(int i) const {
  assert(i < detailsNumberOfSections());
  if (functionIsNonVerticalLine()) {
    constexpr I18n::Message k_descriptions[k_lineDetailsSections] = {
        I18n::Message::LineEquationDescription,
        I18n::Message::LineSlopeDescription,
        I18n::Message::LineYInterceptDescription,
    };
    return k_descriptions[i];
  }
  switch (function()->properties().conicShape()) {
    case Conic::Shape::Circle: {
      constexpr I18n::Message k_descriptions[k_circleDetailsSections] = {
          I18n::Message::CircleRadiusDescription,
          I18n::Message::CenterAbscissaDescription,
          I18n::Message::CenterOrdinateDescription,
      };
      return k_descriptions[i];
    }
    case Conic::Shape::Ellipse: {
      constexpr I18n::Message k_descriptions[k_ellipseDetailsSections] = {
          I18n::Message::EllipseSemiMajorAxisDescription,
          I18n::Message::EllipseSemiMinorAxisDescription,
          I18n::Message::LinearEccentricityDescription,
          I18n::Message::EccentricityDescription,
          I18n::Message::CenterAbscissaDescription,
          I18n::Message::CenterOrdinateDescription,
      };
      return k_descriptions[i];
    }
    case Conic::Shape::Parabola: {
      I18n::Message k_descriptions[k_parabolaDetailsSections] = {
          GlobalPreferences::sharedGlobalPreferences->parabolaParameter() ==
                  Poincare::Preferences::ParabolaParameter::FocalLength
              ? I18n::Message::ParabolaFocalLengthDescription
              : I18n::Message::ParabolaParameterDescription,
          I18n::Message::ParabolaVertexAbscissaDescription,
          I18n::Message::ParabolaVertexOrdinateDescription,
      };
      return k_descriptions[i];
    }
    default: {
      assert(function()->properties().conicShape() == Conic::Shape::Hyperbola);
      constexpr I18n::Message k_descriptions[k_hyperbolaDetailsSections] = {
          I18n::Message::HyperbolaSemiMajorAxisDescription,
          I18n::Message::HyperbolaSemiMinorAxisDescription,
          I18n::Message::LinearEccentricityDescription,
          I18n::Message::EccentricityDescription,
          I18n::Message::CenterAbscissaDescription,
          I18n::Message::CenterOrdinateDescription,
      };
      return k_descriptions[i];
    }
  }
}

void DetailsParameterController::setLineDetailsValues(double slope,
                                                      double intercept) {
  assert(functionIsNonVerticalLine());
  m_detailValues[0] = NAN;
  m_detailValues[1] = slope;
  m_detailValues[2] = intercept;
}

void DetailsParameterController::setConicDetailsValues(Poincare::Conic *conic) {
  Conic::Shape type = function()->properties().conicShape();
  assert(type == conic->conicType().shape);
  double cx, cy;
  if (type == Conic::Shape::Parabola) {
    conic->getSummit(&cx, &cy);
  } else {
    conic->getCenter(&cx, &cy);
  }
  if (type == Conic::Shape::Circle) {
    m_detailValues[0] = conic->getRadius();
    m_detailValues[1] = cx;
    m_detailValues[2] = cy;
    return;
  }
  if (type == Conic::Shape::Ellipse) {
    m_detailValues[0] = conic->getSemiMajorAxis();
    m_detailValues[1] = conic->getSemiMinorAxis();
    m_detailValues[2] = conic->getLinearEccentricity();
    m_detailValues[3] = conic->getEccentricity();
    m_detailValues[4] = cx;
    m_detailValues[5] = cy;
    return;
  }
  if (type == Conic::Shape::Parabola) {
    m_detailValues[0] = conic->getParameter();
    m_detailValues[1] = cx;
    m_detailValues[2] = cy;
    return;
  }
  assert(type == Conic::Shape::Hyperbola);
  m_detailValues[0] = conic->getSemiMajorAxis();
  m_detailValues[1] = conic->getSemiMinorAxis();
  m_detailValues[2] = conic->getLinearEccentricity();
  m_detailValues[3] = conic->getEccentricity();
  m_detailValues[4] = cx;
  m_detailValues[5] = cy;
  return;
}

StackViewController *DetailsParameterController::stackController() const {
  return static_cast<StackViewController *>(parentResponder());
}

}  // namespace Graph
