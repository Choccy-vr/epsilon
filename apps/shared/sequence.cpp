#include "sequence.h"

#include <apps/i18n.h>
#include <apps/shared/poincare_helpers.h>
#include <float.h>
#include <poincare/addition.h>
#include <poincare/based_integer.h>
#include <poincare/code_point_layout.h>
#include <poincare/integer.h>
#include <poincare/layout_helper.h>
#include <poincare/rational.h>
#include <poincare/sequence.h>
#include <poincare/serialization_helper.h>
#include <poincare/sum.h>
#include <poincare/vertical_offset_layout.h>
#include <poincare/zoom.h>
#include <string.h>

#include <cmath>

#include "global_context.h"
#include "sequence_context.h"
#include "sequence_store.h"

using namespace Poincare;

namespace Shared {

I18n::Message Sequence::parameterMessageName() const {
  return I18n::Message::N;
}

size_t Sequence::nameWithArgumentAndType(char *buffer, size_t bufferSize) {
  size_t result = nameWithArgument(buffer, bufferSize);
  assert(result >= 1);
  size_t offset = result - 1;
  switch (type()) {
    case Type::SingleRecurrence:
      result += strlcpy(buffer + offset, "+1)", bufferSize - offset);
      break;
    case Type::DoubleRecurrence:
      result += strlcpy(buffer + offset, "+2)", bufferSize - offset);
      break;
    default:
      break;
  }
  return result;
}

void Sequence::setType(Type t) {
  if (t == type()) {
    return;
  }
  recordData()->setType(t);
  m_definition.tidyName();
  tidyDownstreamPoolFrom();
  /* Reset all contents */
  Ion::Storage::Record::ErrorStatus error =
      Ion::Storage::Record::ErrorStatus::None;
  switch (t) {
    case Type::Explicit:
      error = setContent("", nullptr);  // No context needed here
      break;
    case Type::SingleRecurrence: {
      char ex[] = "u\u0014{n\u0014}";
      /* Maybe in the future sequence names will be longer and this
       * code won't be valid anymore. This assert is to ensure that this code is
       * changed if it's the case.
       * */
      assert(fullName()[1] == 0 || fullName()[1] == '.');
      ex[0] = fullName()[0];
      error = setContent(ex, nullptr);  // No context needed here
      break;
    }
    case Type::DoubleRecurrence: {
      char ex[] = "u\u0014{n+1\u0014}+u\u0014{n\u0014}";
      /* Maybe in the future sequence names will be longer and this
       * code won't be valid anymore. This assert is to ensure that this code is
       * changed if it's the case.
       * */
      assert(fullName()[1] == 0 || fullName()[1] == '.');
      constexpr int k_uNameSecondIndex = 9;
      assert(ex[k_uNameSecondIndex] == 'u');
      char name = fullName()[0];
      ex[0] = name;
      ex[k_uNameSecondIndex] = name;
      error = setContent(ex, nullptr);  // No context needed here
      break;
    }
  }
  assert(error == Ion::Storage::Record::ErrorStatus::None);
  (void)error;                                    // Silence compilation warning
  setFirstInitialConditionContent("", nullptr);   // No context needed here
  setSecondInitialConditionContent("", nullptr);  // No context needed here
}

void Sequence::setInitialRank(int rank) {
  recordData()->setInitialRank(rank);
  m_firstInitialCondition.tidyName();
  m_secondInitialCondition.tidyName();
}

Layout Sequence::nameLayout() {
  return HorizontalLayout::Builder(
      CodePointLayout::Builder(fullName()[0]),
      VerticalOffsetLayout::Builder(
          CodePointLayout::Builder(symbol()),
          VerticalOffsetLayoutNode::VerticalPosition::Subscript));
}

bool Sequence::isDefined() const {
  RecordDataBuffer *data = recordData();
  switch (type()) {
    case Type::Explicit:
      return value().size > metaDataSize();
    case Type::SingleRecurrence:
      return data->initialConditionSize(0) > 0 &&
             value().size > metaDataSize() + data->initialConditionSize(0);
    default:
      return data->initialConditionSize(0) > 0 &&
             data->initialConditionSize(1) > 0 &&
             value().size > metaDataSize() + data->initialConditionSize(0) +
                                data->initialConditionSize(1);
  }
}

bool Sequence::isEmpty() const {
  RecordDataBuffer *data = recordData();
  Type type = data->type();
  return Function::isEmpty() &&
         (type == Type::Explicit || (data->initialConditionSize(0) == 0 &&
                                     (type == Type::SingleRecurrence ||
                                      data->initialConditionSize(1) == 0)));
}

bool Sequence::isSuitableForCobweb(Context *context) const {
  return type() == Type::SingleRecurrence &&
         !std::isnan(approximateAtRank(
             initialRank(), reinterpret_cast<SequenceContext *>(context))) &&
         !mainExpressionContainsForbiddenTerms(context, true, false, false);
}

bool Sequence::mainExpressionContainsForbiddenTerms(
    Context *context, bool recursionIsAllowed, bool systemSymbolIsAllowed,
    bool otherSequencesAreAllowed) const {
  constexpr size_t bufferSize = SequenceStore::k_maxSequenceNameLength + 1;
  char buffer[bufferSize];
  name(buffer, bufferSize);
  struct Pack {
    char *name;
    Type type;
    int initialRank;
    bool recursion;
    bool systemSymbol;
    bool otherSequences;
  };
  Pack pack{buffer,
            type(),
            initialRank(),
            recursionIsAllowed,
            systemSymbolIsAllowed,
            otherSequencesAreAllowed};
  return expressionClone().recursivelyMatches(
      [](const Expression e, Context *context, void *arg) {
        Pack *pack = static_cast<Pack *>(arg);
        if (e.isRandom()) {
          return TrinaryBoolean::True;
        }
        if (!pack->systemSymbol && e.type() == ExpressionNode::Type::Symbol) {
          const Symbol symbol = static_cast<const Symbol &>(e);
          return symbol.isSystemSymbol() ? TrinaryBoolean::True
                                         : TrinaryBoolean::Unknown;
        }
        if (e.type() != ExpressionNode::Type::Sequence) {
          return TrinaryBoolean::Unknown;
        }
        const Poincare::Sequence seq =
            static_cast<const Poincare::Sequence &>(e);
        char *buffer = pack->name;
        if (strcmp(seq.name(), buffer) != 0) {
          return !pack->otherSequences ? TrinaryBoolean::True
                                       : TrinaryBoolean::Unknown;
        }
        Expression rank = seq.childAtIndex(0);
        Type type = pack->type;
        if (rank.type() == ExpressionNode::Type::BasedInteger) {
          float rankValue = static_cast<const BasedInteger &>(rank)
                                .integer()
                                .approximate<float>();
          if ((type != Type::Explicit && rankValue == pack->initialRank) ||
              (type == Type::DoubleRecurrence &&
               rankValue == pack->initialRank + 1)) {
            return TrinaryBoolean::False;
          }
          return TrinaryBoolean::True;
        }
        Symbol n = Symbol::SystemSymbol();
        if (pack->recursion &&
            ((type != Type::Explicit && rank.isIdenticalTo(n)) ||
             (type == Type::DoubleRecurrence &&
              rank.isIdenticalTo(
                  Addition::Builder(n, BasedInteger::Builder(1)))))) {
          return TrinaryBoolean::False;
        }
        return TrinaryBoolean::True;
      },
      context, SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition,
      &pack);
}

void Sequence::tidyDownstreamPoolFrom(TreeNode *treePoolCursor) const {
  model()->tidyDownstreamPoolFrom(treePoolCursor);
  m_firstInitialCondition.tidyDownstreamPoolFrom(treePoolCursor);
  m_secondInitialCondition.tidyDownstreamPoolFrom(treePoolCursor);
}

template <typename T>
T Sequence::privateEvaluateYAtX(T x, Context *context) const {
  // Round behaviour changes platform-wise if std::isnan(x)
  assert(!std::isnan(x));
  int n = std::round(x);
  return static_cast<T>(
      approximateAtRank(n, reinterpret_cast<SequenceContext *>(context)));
}

double Sequence::approximateAtRank(int rank, SequenceContext *sqctx) const {
  int sequenceIndex = SequenceStore::SequenceIndexForName(fullName()[0]);
  if (!isDefined() || rank < initialRank() ||
      (rank >= firstNonInitialRank() &&
       sqctx->sequenceIsNotComputable(sequenceIndex))) {
    return NAN;
  }
  sqctx->stepUntilRank(sequenceIndex, rank);
  return sqctx->storedValueOfSequenceAtRank(sequenceIndex, rank);
}

double Sequence::approximateAtContextRank(SequenceContext *sqctx,
                                          bool intermediateComputation) const {
  int sequenceIndex = SequenceStore::SequenceIndexForName(fullName()[0]);
  int rank = sqctx->rank(sequenceIndex, intermediateComputation);
  if (rank < initialRank()) {
    return NAN;
  }
  double x;
  Expression e;
  if (rank >= firstNonInitialRank()) {
    x = static_cast<double>(rank - order());
    e = expressionReduced(sqctx);
  } else {
    assert(type() != Type::Explicit);
    x = static_cast<double>(NAN);
    if (rank == initialRank()) {
      e = firstInitialConditionExpressionReduced(sqctx);
    } else {
      assert(type() == Type::DoubleRecurrence);
      e = secondInitialConditionExpressionReduced(sqctx);
    }
  }
  // Update angle unit and complex format
  ApproximationContext approximationContext(sqctx, complexFormat(sqctx));
  return e.approximateToScalarWithValueForSymbol(k_unknownName, x,
                                                 approximationContext);
}

Expression Sequence::sumBetweenBounds(double start, double end,
                                      Context *context) const {
  /* Here, we cannot just create the expression sum(u(n), start, end) because
   * the approximation of u(n) is not handled by Poincare (but only by
   * Sequence). */
  double result = 0.0;
  if (end - start > ExpressionNode::k_maxNumberOfSteps ||
      start + 1.0 == start) {
    return Float<double>::Builder(NAN);
  }
  start = std::round(start);
  end = std::round(end);
  for (double i = start; i <= end; i = i + 1.0) {
    /* When |start| >> 1.0, start + 1.0 = start. In that case, quit the
     * infinite loop. */
    if (i == i - 1.0 || i == i + 1.0) {
      return Float<double>::Builder(NAN);
    }
    result += evaluateXYAtParameter(i, context).y();
  }
  return Float<double>::Builder(result);
}

Sequence::RecordDataBuffer *Sequence::recordData() const {
  assert(!isNull());
  Ion::Storage::Record::Data d = value();
  return reinterpret_cast<RecordDataBuffer *>(const_cast<void *>(d.buffer));
}

/* Sequence Model */

Layout Sequence::SequenceModel::name(Sequence *sequence) {
  if (m_name.isUninitialized()) {
    buildName(sequence);
  }
  return m_name;
}

void Sequence::SequenceModel::tidyName(TreeNode *treePoolCursor) const {
  if (treePoolCursor == nullptr || m_name.isDownstreamOf(treePoolCursor)) {
    m_name = Layout();
  }
}

void Sequence::SequenceModel::tidyDownstreamPoolFrom(
    TreeNode *treePoolCursor) const {
  tidyName(treePoolCursor);
  ExpressionModel::tidyDownstreamPoolFrom(treePoolCursor);
}

void Sequence::SequenceModel::updateNewDataWithExpression(
    Ion::Storage::Record *record, const Expression &expressionToStore,
    void *expressionAddress, size_t newExpressionSize,
    size_t previousExpressionSize) {
  Ion::Storage::Record::Data newData = record->value();
  // Translate expressions located downstream
  size_t sizeBeforeExpression =
      (char *)expressionAddress - (char *)newData.buffer;
  size_t remainingSize =
      newData.size - sizeBeforeExpression - previousExpressionSize;
  memmove((char *)expressionAddress + newExpressionSize,
          (char *)expressionAddress + previousExpressionSize, remainingSize);
  // Copy the expression
  if (!expressionToStore.isUninitialized()) {
    memmove(expressionAddress, expressionToStore.addressInPool(),
            newExpressionSize);
  }
  // Update meta data
  updateMetaData(record, newExpressionSize);
}

void Sequence::SequenceModel::setStorageChangeFlag() const {
  GlobalContext::sequenceStore->setStorageChangeFlag(true);
}

/* Definition Handle*/

void *Sequence::DefinitionModel::expressionAddress(
    const Ion::Storage::Record *record) const {
  return (char *)record->value().buffer + sizeof(RecordDataBuffer);
}

size_t Sequence::DefinitionModel::expressionSize(
    const Ion::Storage::Record *record) const {
  Ion::Storage::Record::Data data = record->value();
  RecordDataBuffer *dataBuffer =
      static_cast<const Sequence *>(record)->recordData();
  return data.size - sizeof(RecordDataBuffer) -
         dataBuffer->initialConditionSize(0) -
         dataBuffer->initialConditionSize(1);
}

void Sequence::DefinitionModel::buildName(Sequence *sequence) {
  char name = sequence->fullName()[0];
  if (sequence->type() == Type::Explicit) {
    m_name = HorizontalLayout::Builder(
        CodePointLayout::Builder(name),
        VerticalOffsetLayout::Builder(
            LayoutHelper::String("n", 1),
            VerticalOffsetLayoutNode::VerticalPosition::Subscript));
  } else if (sequence->type() == Type::SingleRecurrence) {
    m_name = HorizontalLayout::Builder(
        CodePointLayout::Builder(name),
        VerticalOffsetLayout::Builder(
            LayoutHelper::String("n+1", 3),
            VerticalOffsetLayoutNode::VerticalPosition::Subscript));
  } else {
    assert(sequence->type() == Type::DoubleRecurrence);
    m_name = HorizontalLayout::Builder(
        CodePointLayout::Builder(name),
        VerticalOffsetLayout::Builder(
            LayoutHelper::String("n+2", 3),
            VerticalOffsetLayoutNode::VerticalPosition::Subscript));
  }
}

/* Initial Condition Handle*/

void *Sequence::InitialConditionModel::expressionAddress(
    const Ion::Storage::Record *record) const {
  Ion::Storage::Record::Data data = record->value();
  RecordDataBuffer *dataBuffer =
      static_cast<const Sequence *>(record)->recordData();
  size_t offset = conditionIndex() == 0
                      ? data.size - dataBuffer->initialConditionSize(0) -
                            dataBuffer->initialConditionSize(1)
                      : data.size - dataBuffer->initialConditionSize(1);
  return (char *)data.buffer + offset;
}

size_t Sequence::InitialConditionModel::expressionSize(
    const Ion::Storage::Record *record) const {
  return static_cast<const Sequence *>(record)
      ->recordData()
      ->initialConditionSize(conditionIndex());
}

void Sequence::InitialConditionModel::updateMetaData(
    const Ion::Storage::Record *record, size_t newSize) {
  static_cast<const Sequence *>(record)->recordData()->setInitialConditionSize(
      newSize, conditionIndex());
}

void Sequence::InitialConditionModel::buildName(Sequence *sequence) {
  assert(
      (conditionIndex() == 0 && sequence->type() == Type::SingleRecurrence) ||
      sequence->type() == Type::DoubleRecurrence);
  char buffer[k_initialRankNumberOfDigits + 1];
  Integer(sequence->initialRank() + conditionIndex())
      .serialize(buffer, k_initialRankNumberOfDigits + 1);
  Layout indexLayout = LayoutHelper::String(buffer, strlen(buffer));
  m_name = HorizontalLayout::Builder(
      CodePointLayout::Builder(sequence->fullName()[0]),
      VerticalOffsetLayout::Builder(
          indexLayout, VerticalOffsetLayoutNode::VerticalPosition::Subscript));
}

template double Sequence::privateEvaluateYAtX<double>(double, Context *) const;
template float Sequence::privateEvaluateYAtX<float>(float, Context *) const;

}  // namespace Shared
