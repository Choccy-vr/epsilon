#include <assert.h>
#include <ion/unicode/utf8_decoder.h>
#include <ion/unicode/utf8_helper.h>
#include <poincare/code_point_layout.h>
#include <poincare/context.h>
#include <poincare/horizontal_layout.h>
#include <poincare/layout_helper.h>
#include <poincare/parametered_expression.h>
#include <poincare/rational.h>
#include <poincare/symbol.h>
#include <poincare/undefined.h>
#include <poincare/vertical_offset_layout.h>
#include <string.h>

#include <cmath>

namespace Poincare {

int SymbolNode::polynomialDegree(Context* context,
                                 const char* symbolName) const {
  if (strcmp(m_name, symbolName) == 0) {
    // This is the symbol we are looking for.
    return 1;
  }
  /* No variable expansion is expected within this method. Only functions are
   * expanded and replaced. */
  return 0;
}

int SymbolNode::getPolynomialCoefficients(Context* context,
                                          const char* symbolName,
                                          Expression coefficients[]) const {
  return Symbol(this).getPolynomialCoefficients(context, symbolName,
                                                coefficients);
}

int SymbolNode::getVariables(Context* context, isVariableTest isVariable,
                             char* variables, int maxSizeVariable,
                             int nextVariableIndex) const {
  /* No variable expansion is expected within this method. Only functions are
   * expanded and replaced. */
  if (isVariable(m_name, context)) {
    int index = 0;
    /* variables is in fact of type
     * char[k_maxNumberOfVariables][maxSizeVariable] */
    while (index < maxSizeVariable * Expression::k_maxNumberOfVariables &&
           variables[index] != 0) {
      if (strcmp(m_name, &variables[index]) == 0) {
        return nextVariableIndex;
      }
      index += maxSizeVariable;
    }
    if (nextVariableIndex < Expression::k_maxNumberOfVariables) {
      assert(variables[nextVariableIndex * maxSizeVariable] == 0);
      if (strlen(m_name) + 1 > (size_t)maxSizeVariable) {
        return -2;
      }
      strlcpy(&variables[nextVariableIndex * maxSizeVariable], m_name,
              maxSizeVariable);
      nextVariableIndex++;
      if (nextVariableIndex < Expression::k_maxNumberOfVariables) {
        variables[nextVariableIndex * maxSizeVariable] = 0;
      }
      return nextVariableIndex;
    }
    return -1;
  }
  return nextVariableIndex;
}

Layout SymbolNode::createLayout(Preferences::PrintFloatMode floatDisplayMode,
                                int numberOfSignificantDigits,
                                Context* context) const {
  assert(!isSystemSymbol());
  return LayoutHelper::String(m_name, strlen(m_name));
}

Expression SymbolNode::shallowReduce(const ReductionContext& reductionContext) {
  return Symbol(this).shallowReduce(reductionContext);
}

Expression SymbolNode::deepReplaceReplaceableSymbols(
    Context* context, TrinaryBoolean* isCircular, int parameteredAncestorsCount,
    SymbolicComputation symbolicComputation) {
  return Symbol(this).deepReplaceReplaceableSymbols(
      context, isCircular, parameteredAncestorsCount, symbolicComputation);
}

bool SymbolNode::derivate(const ReductionContext& reductionContext,
                          Symbol symbol, Expression symbolValue) {
  return Symbol(this).derivate(reductionContext, symbol, symbolValue);
}

template <typename T>
Evaluation<T> SymbolNode::templatedApproximate(
    const ApproximationContext& approximationContext) const {
  Symbol s(this);
  // No need to preserve undefined symbols because they will be approximated.
  Expression e = SymbolAbstract::Expand(
      s, approximationContext.context(), false,
      SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined);
  if (e.isUninitialized()) {
    return Complex<T>::Undefined();
  }
  return e.node()->approximate(T(), approximationContext);
}

bool SymbolNode::isSystemSymbol() const {
  bool result = UTF8Helper::CodePointIs(m_name, UCodePointUnknown);
  if (result) {
    assert(m_name[1] == 0);
  }
  return result;
}

Symbol Symbol::Builder(CodePoint name) {
  constexpr size_t bufferSize = CodePoint::MaxCodePointCharLength + 1;
  char buffer[bufferSize];
  size_t codePointLength =
      SerializationHelper::CodePoint(buffer, bufferSize - 1, name);
  assert(codePointLength < bufferSize);
  return Symbol::Builder(buffer, codePointLength);
}

Expression Symbol::shallowReduce(ReductionContext reductionContext) {
  SymbolicComputation symbolicComputation =
      reductionContext.symbolicComputation();
  if (symbolicComputation ==
          SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions ||
      symbolicComputation == SymbolicComputation::DoNotReplaceAnySymbol) {
    return *this;
  }
  if (symbolicComputation ==
      SymbolicComputation::ReplaceAllSymbolsWithUndefined) {
    return replaceWithUndefinedInPlace();
  }
  assert(symbolicComputation ==
             SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined ||
         symbolicComputation ==
             SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition);
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
        if (index == ParameteredExpression::ParameteredChildIndex() &&
            hasSameNameAs(static_cast<ParameteredExpression&>(p).parameter())) {
          return *this;
        }
      }
      current = p;
      p = current.parent();
    }
  }

  /* Recursively replace symbols and catch circular references involving symbols
   * as well as functions. */
  Expression result = SymbolAbstract::Expand(*this, reductionContext.context(),
                                             true, symbolicComputation);
  if (result.isUninitialized()) {
    if (symbolicComputation ==
        SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition) {
      return *this;
    }
    assert(symbolicComputation ==
           SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined);
    return replaceWithUndefinedInPlace();
  }
  replaceWithInPlace(result);

  /* At this point, any remaining symbol in result is a parameter of a
   * parametered function nested in this expression, (such as 'diff(x,x,2)' in
   * previous example) and it must be preserved.
   * ReductionContext's SymbolicComputation is altered, enforcing preservation
   * of remaining variables only to save computation that has already been
   * done in SymbolAbstract::Expand, when looking for parametered functions. */
  reductionContext.setSymbolicComputation(
      SymbolicComputation::DoNotReplaceAnySymbol);
  // The stored expression is as entered by the user, so we need to call reduce
  result = result.deepReduce(reductionContext);
  return result;
}

bool Symbol::derivate(const ReductionContext& reductionContext, Symbol symbol,
                      Expression symbolValue) {
  replaceWithInPlace(Rational::Builder(strcmp(name(), symbol.name()) == 0));
  return true;
}

int Symbol::getPolynomialCoefficients(Context* context, const char* symbolName,
                                      Expression coefficients[]) const {
  int deg = polynomialDegree(context, symbolName);
  if (deg == 1) {
    coefficients[0] = Rational::Builder(0);
    coefficients[1] = Rational::Builder(1);
  } else {
    assert(deg == 0);
    coefficients[0] = clone();
  }
  return deg;
}

Expression Symbol::deepReplaceReplaceableSymbols(
    Context* context, TrinaryBoolean* isCircular, int parameteredAncestorsCount,
    SymbolicComputation symbolicComputation) {
  /* This symbolic computation parameters make no sense in this method.
   * It is therefore not handled. */
  assert(symbolicComputation != SymbolicComputation::DoNotReplaceAnySymbol);
  if (symbolicComputation ==
      SymbolicComputation::ReplaceAllSymbolsWithUndefined) {
    return replaceWithUndefinedInPlace();
  }
  if (symbolicComputation ==
          SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions ||
      isSystemSymbol()) {
    return *this;
  }

  assert(symbolicComputation ==
             SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined ||
         symbolicComputation ==
             SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition);

  // Check that this is not a parameter in a parametered expression
  Expression ancestor = *this;
  while (parameteredAncestorsCount > 0) {
    ancestor = ancestor.parent();
    assert(!ancestor.isUninitialized());
    if (ancestor.isParameteredExpression()) {
      parameteredAncestorsCount--;
      Symbol ancestorParameter =
          static_cast<ParameteredExpression&>(ancestor).parameter();
      if (hasSameNameAs(ancestorParameter)) {
        return *this;
      }
    }
  }

  Expression e = context->expressionForSymbolAbstract(*this, true);
  if (e.isUninitialized()) {
    if (symbolicComputation ==
        SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition) {
      return *this;
    }
    assert(symbolicComputation ==
           SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined);
    return replaceWithUndefinedInPlace();
  }

  /* A symbol outside parametered expressions is not supposed to depend on
   * another symbol, the latter is directly replaced by its expression (cf
   * Store::deepReduceChildren). If we decide to allow symbols to depend on
   * other symbols, then circularity should be checked like in
   * Function::involvesCircularity. */
  assert(
      !e.deepIsSymbolic(nullptr, SymbolicComputation::DoNotReplaceAnySymbol));

  replaceWithInPlace(e);
  /* Reset parameteredAncestorsCount, because outer local context is ignored
   * within symbol's expression. */
  e = e.deepReplaceReplaceableSymbols(context, isCircular, 0,
                                      symbolicComputation);
  assert(*isCircular != TrinaryBoolean::True);
  return e;
}

}  // namespace Poincare
