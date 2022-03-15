#include <quiz.h>
#include "helper.h"
#include <apps/shared/global_context.h>

using namespace Shared;

namespace Graph {

void assert_check_function_properties(const char * expression, ContinuousFunction::PlotType plotType, ExpressionNode::Type expectedEquationType = ExpressionNode::Type::Equal, ContinuousFunction::AreaType expectedAreaType = ContinuousFunction::AreaType::None) {
  GlobalContext context;
  ContinuousFunctionStore store;
  // AddFunction asserts the function is of the expected plotType.
  Shared::ContinuousFunction * function = addFunction(expression, plotType, &store, &context);
  // Memoize the reduced expression so that numberOfSubCurves() can be asserted
  function->expressionReduced(&context);
  if (!ContinuousFunction::IsPlotTypeInactive(plotType)) {
    ExpressionNode::Type observedEquationType = function->equationType();
    ContinuousFunction::AreaType observedAreaType = function->areaType();
    // EquationType is accurate on active plot types only.
    quiz_assert(observedEquationType == expectedEquationType);
    quiz_assert(observedAreaType == expectedAreaType);
  }
  store.removeAll();
}

QUIZ_CASE(graph_function_plot_type) {
  // Test the plot type under different Press-to-test parameters :
  Preferences::PressToTestParams pressToTestParams = Preferences::PressToTestParams({0});
  for (size_t config = 0; config < 4; config++) {
    bool noInequations = (config == 1 || config == 3);
    bool noImplicitPlot = (config == 2 || config == 3);
    // Set the Press-to-test mode
    pressToTestParams.m_inequalityGraphingIsForbidden = noInequations;
    pressToTestParams.m_implicitPlotsAreForbidden = noImplicitPlot;
    Poincare::Preferences::sharedPreferences()->setPressToTestParams(pressToTestParams);
    Poincare::Preferences::sharedPreferences()->setExamMode((noInequations || noImplicitPlot) ? Poincare::Preferences::ExamMode::PressToTest : Poincare::Preferences::ExamMode::Off);

    assert_check_function_properties("f(θ)=2", ContinuousFunction::PlotType::Polar);
    assert_check_function_properties("g(t)=[[cos(t)][t]]", ContinuousFunction::PlotType::Parametric);
    assert_check_function_properties("h(x)=log(x)", ContinuousFunction::PlotType::Cartesian);
    assert_check_function_properties("y=log(x)", ContinuousFunction::PlotType::Cartesian);
    assert_check_function_properties("y+x+1=0", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Line);
    assert_check_function_properties("y=2*y+1", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::HorizontalLine);
    assert_check_function_properties("2=y", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::HorizontalLine);
    assert_check_function_properties("y=0", ContinuousFunction::PlotType::HorizontalLine);
    assert_check_function_properties("x=1", ContinuousFunction::PlotType::VerticalLine);
    assert_check_function_properties("0=x", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::VerticalLine);
    assert_check_function_properties("x=1-x", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::VerticalLine);
    assert_check_function_properties("0=x^2", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::VerticalLines);
    assert_check_function_properties("1=x^2+x", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::VerticalLines);
    assert_check_function_properties("1+x^2=0", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::VerticalLines);
    assert_check_function_properties("x^2<0", (noInequations || noImplicitPlot) ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::VerticalLines, ExpressionNode::Type::Inferior, ContinuousFunction::AreaType::Inside);
    assert_check_function_properties("y>log(x)", noInequations ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Cartesian, ExpressionNode::Type::Superior, ContinuousFunction::AreaType::Above);
    assert_check_function_properties("2-y>log(x)", (noInequations || noImplicitPlot) ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Cartesian, ExpressionNode::Type::Inferior, ContinuousFunction::AreaType::Below);
    assert_check_function_properties("2-y^2>x^2+x+y", (noInequations || noImplicitPlot) ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Circle, ExpressionNode::Type::Inferior, ContinuousFunction::AreaType::Inside);
    assert_check_function_properties("p(x)>log(x)", noInequations ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Cartesian, ExpressionNode::Type::Superior, ContinuousFunction::AreaType::Above);
    assert_check_function_properties("x^2+y^2=12", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Circle);
    assert_check_function_properties("x^2+2*y^2=12", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Ellipse);
    assert_check_function_properties("x=y^2", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Parabola);
    /* When implicit plots are disabled, these conics are no longer identified
     * to hide details */
    assert_check_function_properties("y=x^2", noImplicitPlot ? ContinuousFunction::PlotType::Cartesian : ContinuousFunction::PlotType::CartesianParabola);

    assert_check_function_properties("x^2-2*y^2=12", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Hyperbola);
    assert_check_function_properties("y*x=1", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::CartesianHyperbola);
    assert_check_function_properties("x^2-y^2=0", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Other);
    assert_check_function_properties("x*y^2=1", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Other);
    assert_check_function_properties("x^2-y^2+log(x)=0", noImplicitPlot ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Other);
    assert_check_function_properties("y^2>-1", (noInequations || noImplicitPlot) ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Other, ExpressionNode::Type::Superior, ContinuousFunction::AreaType::Outside);
    assert_check_function_properties("(y-x+x^2)^2>=0", (noInequations || noImplicitPlot) ? ContinuousFunction::PlotType::Disabled : ContinuousFunction::PlotType::Other, ExpressionNode::Type::SuperiorEqual, ContinuousFunction::AreaType::Outside);
    assert_check_function_properties("", ContinuousFunction::PlotType::Undefined);
    assert_check_function_properties("y=log(0)", ContinuousFunction::PlotType::Undefined);
    assert_check_function_properties("f(t)=[[cos(t)][t]]*log(0)", ContinuousFunction::PlotType::UndefinedParametric);
    assert_check_function_properties("f(θ)=θ+[[0][0]]", ContinuousFunction::PlotType::UndefinedPolar);
    assert_check_function_properties("3=2", ContinuousFunction::PlotType::Unhandled);
    assert_check_function_properties("y^3=x", ContinuousFunction::PlotType::Unhandled);
    assert_check_function_properties("x*y^2>1", ContinuousFunction::PlotType::Unhandled);
    assert_check_function_properties("2-y^2>log(x)", ContinuousFunction::PlotType::Unhandled);
    assert_check_function_properties("x*y^2=x", ContinuousFunction::PlotType::Unhandled);
    assert_check_function_properties("y=𝐢*x+1", ContinuousFunction::PlotType::Unhandled);
    // TODO : Handle this function
    assert_check_function_properties("y=im(𝐢*x+1)", ContinuousFunction::PlotType::Unhandled);
  }
  // Restore an Off exam mode.
  Poincare::Preferences::sharedPreferences()->setPressToTestParams(Preferences::PressToTestParams({0}));
  Poincare::Preferences::sharedPreferences()->setExamMode(Poincare::Preferences::ExamMode::Off);
}

QUIZ_CASE(graph_function_plot_type_with_predefined_variables) {
    GlobalContext context;
    ContinuousFunctionStore store;
    // Add a predefined test function
    addFunction("test(x)=1+x", ContinuousFunction::PlotType::Cartesian, &store, &context);

    addFunction("y=x", ContinuousFunction::PlotType::Line, &store, &context);
    addFunction("y=test(x)", ContinuousFunction::PlotType::Line, &store, &context);
    addFunction("y=a*x+1", ContinuousFunction::PlotType::Line, &store, &context);
    addFunction("a*y*y+y=x", ContinuousFunction::PlotType::Other, &store, &context);

    // Add a predefined a symbol
    assert_reduce("0→a", Preferences::AngleUnit::Radian, Poincare::Preferences::UnitFormat::Metric, Poincare::Preferences::ComplexFormat::Real);
    Expression::Parse("0→a", &context);
    addFunction("y=a*x+1", ContinuousFunction::PlotType::HorizontalLine, &store, &context);
    addFunction("a*y*y+y=x", ContinuousFunction::PlotType::Line, &store, &context);

    assert_reduce("1→a", Preferences::AngleUnit::Radian, Poincare::Preferences::UnitFormat::Metric, Poincare::Preferences::ComplexFormat::Real);
    addFunction("y=a*x+1", ContinuousFunction::PlotType::Line, &store, &context);
    addFunction("a*y*y+y=x", ContinuousFunction::PlotType::Parabola, &store, &context);

    // Add a predefined y symbol
    assert_reduce("1→y", Preferences::AngleUnit::Radian, Poincare::Preferences::UnitFormat::Metric, Poincare::Preferences::ComplexFormat::Real);
    addFunction("y=x", ContinuousFunction::PlotType::Line, &store, &context);

    Ion::Storage::sharedStorage()->recordNamed("a.exp").destroy();
    Ion::Storage::sharedStorage()->recordNamed("y.exp").destroy();
    store.removeAll();
}

}
