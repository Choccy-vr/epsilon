#include <poincare/symbol.h>
#include <poincare/code_point_layout.h>
#include <poincare/context.h>
#include <poincare/horizontal_layout.h>
#include <poincare/layout_helper.h>
#include <poincare/parametered_expression.h>
#include <poincare/parenthesis.h>
#include <poincare/rational.h>
#include <poincare/undefined.h>
#include <poincare/vertical_offset_layout.h>
#include <ion/unicode/utf8_decoder.h>
#include <ion/unicode/utf8_helper.h>
#include <cmath>
#include <string.h>
#include <assert.h>

namespace Poincare {

constexpr char Symbol::k_ans[];

SymbolNode::SymbolNode(const char * newName, int length) : SymbolAbstractNode() {
  strlcpy(const_cast<char*>(name()), newName, length+1);
}

Expression SymbolNode::replaceSymbolWithExpression(const SymbolAbstract & symbol, const Expression & expression) {
  return Symbol(this).replaceSymbolWithExpression(symbol, expression);
}

int SymbolNode::polynomialDegree(Context * context, const char * symbolName) const {
  if (strcmp(m_name, symbolName) == 0) {
    // This is the symbol we are looking for.
    return 1;
  }
  /* No variable expansion is expected within this method. Only functions are
   * expanded and replaced. */
  return 0;
}

int SymbolNode::getPolynomialCoefficients(Context * context, const char * symbolName, Expression coefficients[]) const {
  return Symbol(this).getPolynomialCoefficients(context, symbolName, coefficients);
}

int SymbolNode::getVariables(Context * context, isVariableTest isVariable, char * variables, int maxSizeVariable, int nextVariableIndex) const {
  /* No variable expansion is expected within this method. Only functions are
   * expanded and replaced. */
  if (isVariable(m_name, context)) {
    int index = 0;
    // variables is in fact of type char[k_maxNumberOfVariables][maxSizeVariable]
    while (index < maxSizeVariable*Expression::k_maxNumberOfVariables && variables[index] != 0) {
      if (strcmp(m_name, &variables[index]) == 0) {
        return nextVariableIndex;
      }
      index+= maxSizeVariable;
    }
    if (nextVariableIndex < Expression::k_maxNumberOfVariables) {
      assert(variables[nextVariableIndex*maxSizeVariable] == 0);
      if (strlen(m_name) + 1 > (size_t)maxSizeVariable) {
        return -2;
      }
      strlcpy(&variables[nextVariableIndex*maxSizeVariable], m_name, maxSizeVariable);
      nextVariableIndex++;
      if (nextVariableIndex < Expression::k_maxNumberOfVariables) {
        variables[nextVariableIndex*maxSizeVariable] = 0;
      }
      return nextVariableIndex;
    }
    return -1;
  }
  return nextVariableIndex;
}

Layout SymbolNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  assert(!isUnknown());
  return LayoutHelper::String(m_name, strlen(m_name));
}

Expression SymbolNode::shallowReduce(ReductionContext reductionContext) {
  return Symbol(this).shallowReduce(reductionContext);
}

Expression SymbolNode::deepReplaceReplaceableSymbols(Context * context, bool * isCircular, int maxSymbolsToReplace, int parameteredAncestorsCount, SymbolicComputation symbolicComputation) {
  return Symbol(this).deepReplaceReplaceableSymbols(context, isCircular, maxSymbolsToReplace, parameteredAncestorsCount, symbolicComputation);
}

ExpressionNode::LayoutShape SymbolNode::leftLayoutShape() const {
  UTF8Decoder decoder(m_name);
  decoder.nextCodePoint();
  if (decoder.nextCodePoint() == UCodePointNull) {  // nextCodePoint asserts that the first character is non-null
    return LayoutShape::OneLetter;
  }
  return LayoutShape::MoreLetters;
}

bool SymbolNode::derivate(ReductionContext reductionContext, Symbol symbol, Expression symbolValue) {
  return Symbol(this).derivate(reductionContext, symbol, symbolValue);
}

template<typename T>
Evaluation<T> SymbolNode::templatedApproximate(ApproximationContext approximationContext) const {
  Symbol s(this);
  // No need to preserve undefined symbols because they will be approximated.
  Expression e = SymbolAbstract::Expand(s, approximationContext.context(), false, SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined);
  if (e.isUninitialized()) {
    return Complex<T>::Undefined();
  }
  return e.node()->approximate(T(), approximationContext);
}

bool SymbolNode::isUnknown() const {
  bool result = UTF8Helper::CodePointIs(m_name, UCodePointUnknown);
  if (result) {
    assert(m_name[1] == 0);
  }
  return result;
}

Symbol Symbol::Builder(CodePoint name) {
  constexpr int bufferSize = CodePoint::MaxCodePointCharLength + 1;
  char buffer[bufferSize];
  int codePointLength = UTF8Decoder::CodePointToChars(name, buffer, bufferSize - 1);
  assert(codePointLength < bufferSize);
  buffer[codePointLength] = 0;
  return Symbol::Builder(buffer, codePointLength);
}

bool Symbol::isSeriesSymbol(const char * c, Poincare::Context * context) {
  // [NV][1-3]
  if (c[2] == 0 && (c[0] == 'N' || c[0] == 'V') && c[1] >= '1' && c[1] <= '3') {
    return true;
  }
  return false;
}

bool Symbol::isRegressionSymbol(const char * c, Poincare::Context * context) {
  // [XY][1-3]
  if (c[2] == 0 && (c[0] == 'X' || c[0] == 'Y') && c[1] >= '1' && c[1] <= '3') {
    return true;
  }
  return false;
}

Expression Symbol::shallowReduce(ExpressionNode::ReductionContext reductionContext) {
  ExpressionNode::SymbolicComputation symbolicComputation = reductionContext.symbolicComputation();
  if (symbolicComputation == ExpressionNode::SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions
    || symbolicComputation == ExpressionNode::SymbolicComputation::DoNotReplaceAnySymbol)
  {
    return *this;
  }
  if (symbolicComputation == ExpressionNode::SymbolicComputation::ReplaceAllSymbolsWithUndefined) {
    return replaceWithUndefinedInPlace();
  }
  assert(symbolicComputation == ExpressionNode::SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined
    || symbolicComputation == ExpressionNode::SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition);
  {
    Expression current = *this;
    Expression p = parent();
    while (!p.isUninitialized()) {
      if (p.isParameteredExpression()) {
        int index = p.indexOfChild(current);
        if (index == ParameteredExpression::ParameterChildIndex()) {
          // The symbol is a parametered expression's parameter
          return *this;
        }
        if (index == ParameteredExpression::ParameteredChildIndex()
            && hasSameNameAs(static_cast<ParameteredExpression&>(p).parameter()))
        {
          return *this;
        }
      }
      current = p;
      p = current.parent();
    }
  }

  /* Recursively replace symbols and catch circular references involving symbols
   * as well as functions.
   * A symbol does not behave the same as a function : any nested symbol that
   * is either undefined or was the parameter of a function defined in the
   * parents of this expression must be replaced with undefined.
   * For example, within the expression 'diff(a,x,3)', with 'a' defined as
   * 'diff(x,x,x)' :
   * If x is globally undefined, 'a' should be expanded to 'diff(x,x,undef)'
   * If x is defined as '2', 'a' should be expanded to 'diff(x,x,2)'.
   * Reduction's symbolic computation only apply on non-nested variables. */
  Expression result = SymbolAbstract::Expand(*this, reductionContext.context(), true, ExpressionNode::SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined);
  if (result.isUninitialized()) {
    if (symbolicComputation != ExpressionNode::SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined) {
      return *this;
    }
    return replaceWithUndefinedInPlace();
  }
  replaceWithInPlace(result);

  /* At this point, any remaining symbol in result is a parameter of a
   * parametered function nested in this expression, (such as 'diff(x,x,2)' in
   * previous example) and it must be preserved.
   * ReductionContext's SymbolicComputation is altered, enforcing preservation
   * of remaining variables only to save computation that has already been
   * done in SymbolAbstract::Expand, when looking for parametered functions. */
  reductionContext.setSymbolicComputation(ExpressionNode::SymbolicComputation::DoNotReplaceAnySymbol);
  // The stored expression is as entered by the user, so we need to call reduce
  result = result.deepReduce(reductionContext);
  // Restore symbolic computation
  reductionContext.setSymbolicComputation(symbolicComputation);
  return result;
}

bool Symbol::derivate(ExpressionNode::ReductionContext reductionContext, Symbol symbol, Expression symbolValue) {
  replaceWithInPlace(Rational::Builder(strcmp(name(), symbol.name()) == 0));
  return true;
}

Expression Symbol::replaceSymbolWithExpression(const SymbolAbstract & symbol, const Expression & expression) {
  if (symbol.type() == ExpressionNode::Type::Symbol && hasSameNameAs(symbol)) {
    Expression value = expression.clone();
    Expression p = parent();
    if (!p.isUninitialized() && p.node()->childAtIndexNeedsUserParentheses(value, p.indexOfChild(*this))) {
      value = Parenthesis::Builder(value);
    }
    replaceWithInPlace(value);
    return value;
  }
  return *this;
}

int Symbol::getPolynomialCoefficients(Context * context, const char * symbolName, Expression coefficients[]) const {
  if (strcmp(name(), symbolName) == 0) {
    // This is the symbol we are looking for.
    coefficients[0] = Rational::Builder(0);
    coefficients[1] = Rational::Builder(1);
    return 1;
  }
  /* No variable expansion is expected within this method. Only functions are
   * expanded and replaced. */
  coefficients[0] = clone();
  return 0;
}

Expression Symbol::deepReplaceReplaceableSymbols(Context * context, bool * isCircular, int maxSymbolsToReplace, int parameteredAncestorsCount, ExpressionNode::SymbolicComputation symbolicComputation) {
  /* These two symbolic computations parameters make no sense in this method.
   * They are therefore not handled. */
  assert(symbolicComputation != ExpressionNode::SymbolicComputation::ReplaceAllSymbolsWithUndefined
    && symbolicComputation != ExpressionNode::SymbolicComputation::DoNotReplaceAnySymbol);
  if (symbolicComputation == ExpressionNode::SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions || isSystemSymbol()) {
    return *this;
  }

  // Check that this is not a parameter in a parametered expression
  Expression ancestor = *this;
  while (parameteredAncestorsCount > 0) {
    ancestor = ancestor.parent();
    assert(!ancestor.isUninitialized());
    if (ancestor.isParameteredExpression()) {
      parameteredAncestorsCount--;
      Symbol ancestorParameter = static_cast<ParameteredExpression&>(ancestor).parameter();
      if (hasSameNameAs(ancestorParameter)) {
        return *this;
      }
    }
  }

  Expression e = context->expressionForSymbolAbstract(*this, true);
  if (e.isUninitialized()) {
    if (symbolicComputation != ExpressionNode::SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined) {
      return *this;
    }
    return replaceWithUndefinedInPlace();
  }

  // Symbol is about to be replaced, decrement maxSymbolsToReplace
  maxSymbolsToReplace--;
  if (maxSymbolsToReplace < 0) {
    // We replaced too many symbols and consider the expression to be circular
    *isCircular = true;
    return *this;
  }

  replaceWithInPlace(e);
  /* Reset parameteredAncestorsCount, because outer local context is ignored
   * within symbol's expression. */
  e = e.deepReplaceReplaceableSymbols(context, isCircular, maxSymbolsToReplace, 0, symbolicComputation);
  return e;
}

}
