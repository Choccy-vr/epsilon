#include <ion/unicode/utf8_decoder.h>
#include <ion/unicode/utf8_helper.h>
#include <poincare/complex_cartesian.h>
#include <poincare/constant.h>
#include <poincare/dependency.h>
#include <poincare/function.h>
#include <poincare/helpers.h>
#include <poincare/parenthesis.h>
#include <poincare/rational.h>
#include <poincare/sequence.h>
#include <poincare/symbol.h>
#include <poincare/symbol_abstract.h>
#include <poincare/undefined.h>
#include <string.h>

#include <algorithm>

namespace Poincare {

SymbolAbstractNode::SymbolAbstractNode(const char *newName, int length)
    : ExpressionNode() {
  assert(NameLengthIsValid(newName, length));
  strlcpy(m_name, newName, length + 1);
}

size_t SymbolAbstractNode::NameWithoutQuotationMarks(char *buffer,
                                                     size_t bufferSize,
                                                     const char *name,
                                                     size_t nameLength) {
  if (NameHasQuotationMarks(name, nameLength)) {
    assert(bufferSize > nameLength - 2);
    size_t result = strlcpy(buffer, name + 1, bufferSize) - 1;
    buffer[nameLength - 2] = 0;  // Remove the last '""
    return result;
  }
  assert(bufferSize > nameLength);
  return strlcpy(buffer, name, bufferSize);
}

Expression SymbolAbstractNode::replaceSymbolWithExpression(
    const SymbolAbstract &symbol, const Expression &expression) {
  return SymbolAbstract(this).replaceSymbolWithExpression(symbol, expression);
}

ExpressionNode::LayoutShape SymbolAbstractNode::leftLayoutShape() const {
  UTF8Decoder decoder(m_name);
  decoder.nextCodePoint();
  // nextCodePoint asserts that the first character is non-null
  if (decoder.nextCodePoint() == UCodePointNull) {
    return ExpressionNode::LayoutShape::OneLetter;
  }
  return ExpressionNode::LayoutShape::MoreLetters;
}

size_t SymbolAbstractNode::size() const {
  return nodeSize() + strlen(name()) + 1;
}

TrinaryBoolean SymbolAbstractNode::isPositive(Context *context) const {
  SymbolAbstract s(this);
  // No need to preserve undefined symbols here.
  Expression e = SymbolAbstract::Expand(
      s, context, true,
      SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined);
  if (e.isUninitialized()) {
    return TrinaryBoolean::Unknown;
  }
  return e.isPositive(context);
}

int SymbolAbstractNode::simplificationOrderSameType(
    const ExpressionNode *e, bool ascending, bool ignoreParentheses) const {
  assert(type() == e->type());
  return strcmp(name(), static_cast<const SymbolAbstractNode *>(e)->name());
}

size_t SymbolAbstractNode::serialize(
    char *buffer, size_t bufferSize,
    Preferences::PrintFloatMode floatDisplayMode,
    int numberOfSignificantDigits) const {
  return std::min<size_t>(strlcpy(buffer, name(), bufferSize), bufferSize - 1);
}

template <typename T, typename U>
T SymbolAbstract::Builder(const char *name, int length) {
  if (AliasesLists::k_thetaAliases.contains(name, length)) {
    name = AliasesLists::k_thetaAliases.mainAlias();
    length = strlen(name);
  }
  size_t size = sizeof(U) + length + 1;
  void *bufferNode = TreePool::sharedPool->alloc(size);
  U *node = new (bufferNode) U(name, length);
  assert(node->size() == size);
  TreeHandle h = TreeHandle::BuildWithGhostChildren(node);
  return static_cast<T &>(h);
}

bool SymbolAbstract::hasSameNameAs(const SymbolAbstract &other) const {
  return strcmp(other.name(), name()) == 0;
}

bool SymbolAbstract::matches(const SymbolAbstract &symbol,
                             ExpressionTrinaryTest test, Context *context,
                             void *auxiliary,
                             Expression::IgnoredSymbols *ignoredSymbols) {
  // Undefined symbols must be preserved.
  Expression e = SymbolAbstract::Expand(
      symbol, context, true,
      SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition);
  return !e.isUninitialized() &&
         e.recursivelyMatches(test, context,
                              SymbolicComputation::DoNotReplaceAnySymbol,
                              auxiliary, ignoredSymbols);
}

Expression SymbolAbstract::replaceSymbolWithExpression(
    const SymbolAbstract &symbol, const Expression &expression) {
  deepReplaceSymbolWithExpression(symbol, expression);
  if (symbol.type() == type() && hasSameNameAs(symbol)) {
    Expression exp = expression.clone();
    if (numberOfChildren() > 0) {
      assert(isOfType(
          {ExpressionNode::Type::Function, ExpressionNode::Type::Sequence}));
      assert(numberOfChildren() == 1 && symbol.numberOfChildren() == 1);
      Expression myVariable = childAtIndex(0).clone();
      Expression symbolVariable = symbol.childAtIndex(0);
      if (symbolVariable.type() == ExpressionNode::Type::Symbol) {
        exp = exp.replaceSymbolWithExpression(symbolVariable.convert<Symbol>(),
                                              myVariable);
      } else if (!myVariable.isIdenticalTo(symbolVariable)) {
        return *this;
      }
    }
    Expression p = parent();
    if (!p.isUninitialized() && p.node()->childAtIndexNeedsUserParentheses(
                                    exp, p.indexOfChild(*this))) {
      exp = Parenthesis::Builder(exp);
    }
    replaceWithInPlace(exp);
    return exp;
  }
  return *this;
}

Expression SymbolAbstract::Expand(const SymbolAbstract &symbol,
                                  Context *context, bool clone,
                                  SymbolicComputation symbolicComputation) {
  assert(context);
  Expression e = context->expressionForSymbolAbstract(symbol, clone);
  /* Replace all the symbols iteratively. This prevents a memory failure when
   * symbols are defined circularly. Symbols defined in a parametered function
   * will be preserved as long as the function is defined within this symbol. */
  e = Expression::ExpressionWithoutSymbols(e, context, symbolicComputation);
  if (!e.isUninitialized() && symbol.type() == ExpressionNode::Type::Function) {
    Dependency d = Dependency::Builder(e);
    d.addDependency(symbol.childAtIndex(0));
    return std::move(d);
  }
  return e;
}

template Constant SymbolAbstract::Builder<Constant, ConstantNode>(char const *,
                                                                  int);
template Function SymbolAbstract::Builder<Function, FunctionNode>(char const *,
                                                                  int);
template Sequence SymbolAbstract::Builder<Sequence, SequenceNode>(char const *,
                                                                  int);
template Symbol SymbolAbstract::Builder<Symbol, SymbolNode>(char const *, int);

}  // namespace Poincare
