#include "calculation_store.h"
#include "../shared/poincare_helpers.h"
#include <poincare/circuit_breaker_checkpoint.h>
#include <poincare/rational.h>
#include <poincare/symbol.h>
#include <poincare/undefined.h>
#include "../exam_mode_configuration.h"
#include <assert.h>

using namespace Poincare;
using namespace Shared;

namespace Calculation {

CalculationStore::CalculationStore(char * buffer, int size) :
  m_buffer(buffer),
  m_bufferSize(size),
  m_calculationAreaEnd(m_buffer),
  m_numberOfCalculations(0)
{
  assert(m_buffer != nullptr);
  assert(m_bufferSize > 0);
}

// Returns an expiring pointer to the calculation of index i
ExpiringPointer<Calculation> CalculationStore::calculationAtIndex(int i) {
  assert(i >= 0 && i < m_numberOfCalculations);
  // m_buffer is the adress of the oldest calculation in calculation store
  Calculation * c = (Calculation *) m_buffer;
  if (i != m_numberOfCalculations-1) {
    // The calculation we want is not the oldest one so we get its pointer
    c = *reinterpret_cast<Calculation**>(addressOfPointerToCalculationOfIndex(i+1));
  }
  return ExpiringPointer<Calculation>(c);
}

// Pushes an expression in the store
ExpiringPointer<Calculation> CalculationStore::push(const char * text, Context * context, HeightComputer heightComputer) {
  /* TODO: we could refine this UserCircuitBreaker. When interrupted during
   * simplification, we could still try to display the approximate result? When
   * interrupted during approximation, we could at least display the exact
   * result. If we do so, don't forget to force the Calculation sign to be
   * approximative to avoid long computation to determine it.
   */

  /* Store information about the buffer state to be able to reset it in case of
   * interruption */
  const char * calculationAreaEnd = m_calculationAreaEnd;
  int numberOfCalculations = m_numberOfCalculations;

  UserCircuitBreakerCheckpoint checkpoint;
  if (CircuitBreakerRun(checkpoint)) {
    /* Compute ans now, before the buffer is updated and before the calculation
     * might be deleted */
    Expression ans = ansExpression(context);

    /* Prepare the buffer for the new calculation.
     * The minimal size to store the new calculation is the minimal size of a
     * calculation plus the pointer to its end */
    constexpr int minimalSize = Calculation::k_minimalSize + sizeof(Calculation *);
    assert(m_bufferSize > minimalSize);
    while (remainingBufferSize() < minimalSize) {
      // If there is no more space to store a calculation, we delete the oldest one
      deleteOldestCalculation();
      // Update the calculation buffer state
      calculationAreaEnd = m_calculationAreaEnd;
      numberOfCalculations = m_numberOfCalculations;
    }

    // Getting the adresses of the limits of the free space
    char * beginingOfFreeSpace = (char *)m_calculationAreaEnd;
    char * endOfFreeSpace = beginingOfMemoizationArea();
    char * previousCalc = beginingOfFreeSpace;

    // Add the beginning of the calculation
    {
      /* Copy the begining of the calculation. The calculation minimal size is
       * available, so this memmove will not overide anything. */
      Calculation newCalc = Calculation();
      size_t calcSize = sizeof(newCalc);
      memcpy(beginingOfFreeSpace, &newCalc, calcSize);
      beginingOfFreeSpace += calcSize;
    }

    /* Add the input expression.
     * We do not store directly the text entered by the user because we do not
     * want to keep Ans symbol in the calculation store. */
    const char * inputSerialization = beginingOfFreeSpace;
    {
      Expression input = Expression::Parse(text, context).replaceSymbolWithExpression(Symbol::Ans(), ans);
      if (!pushSerializedExpression(input, &beginingOfFreeSpace, endOfFreeSpace)) {
        // Update the calculation buffer state
        calculationAreaEnd = m_buffer;
        numberOfCalculations = 0;
        /* If the input does not fit in the store (event if the current
         * calculation is the only calculation), just replace the calculation with
         * undef. */
        return emptyStoreAndPushUndef(context, heightComputer);
      }
    }

    // Compute and serialize the outputs
    /* The serialized outputs are:
     * - the exact ouput
     * - the approximate output with the maximal number of significant digits
     * - the approximate output with the displayed number of significant digits */
    {
      // Outputs hold exact output, approximate output and its duplicate
      constexpr static int numberOfOutputs = Calculation::k_numberOfExpressions - 1;
      Expression outputs[numberOfOutputs] = {Expression(), Expression(), Expression()};
      PoincareHelpers::ParseAndSimplifyAndApproximate(inputSerialization, &(outputs[0]), &(outputs[1]), context, Poincare::ExpressionNode::SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined);
      if (ExamModeConfiguration::exactExpressionIsForbidden(GlobalPreferences::sharedGlobalPreferences()->examMode(), outputs[1]) && outputs[1].hasUnit()) {
        // Hide results with units on units if required by the exam mode configuration
        outputs[1] = Undefined::Builder();
      }
      outputs[2] = outputs[1];
      int numberOfSignificantDigits = Poincare::PrintFloat::k_numberOfStoredSignificantDigits;
      for (int i = 0; i < numberOfOutputs; i++) {
        if (i == numberOfOutputs - 1) {
          numberOfSignificantDigits = Poincare::Preferences::sharedPreferences()->numberOfSignificantDigits();
        }
        if (!pushSerializedExpression(outputs[i], &beginingOfFreeSpace, endOfFreeSpace, numberOfSignificantDigits)) {
          /* If the exact/approximate output does not fit in the store (even if the
           * current calculation is the only one), replace the output with
           * undef if it fits, else replace the whole calculation with undef. */
          Expression undef = Undefined::Builder();
          if (!pushSerializedExpression(undef, &beginingOfFreeSpace, endOfFreeSpace)) {
            // Update the calculation buffer state
            calculationAreaEnd = m_buffer;
            numberOfCalculations = 0;
            return emptyStoreAndPushUndef(context, heightComputer);
          }
        }
      }
    }
    // Storing the pointer of the end of the new calculation
    memcpy(endOfFreeSpace-sizeof(Calculation*),&beginingOfFreeSpace,sizeof(beginingOfFreeSpace));

    // The new calculation is now stored
    m_numberOfCalculations++;

    // The end of the calculation storage area is updated
    m_calculationAreaEnd += beginingOfFreeSpace - previousCalc;
    ExpiringPointer<Calculation> calculation = ExpiringPointer<Calculation>(reinterpret_cast<Calculation *>(previousCalc));
    /* Heights are computed now to make sure that the display output is decided
     * accordingly to the remaining size in the Poincare pool. Once it is, it
     * can't change anymore: the calculation heights are fixed which ensures that
     * scrolling computation is right. */
    calculation->setHeights(
        heightComputer(calculation.pointer(), context, false),
        heightComputer(calculation.pointer(), context, true));
    return calculation;
  } else {
    m_calculationAreaEnd = calculationAreaEnd;
    m_numberOfCalculations = numberOfCalculations;
    return nullptr;
  }
}

// Delete the calculation of index i
void CalculationStore::deleteCalculationAtIndex(int i) {
  assert(i >= 0 && i < m_numberOfCalculations);
  if (i == 0) {
    ExpiringPointer<Calculation> lastCalculationPointer = calculationAtIndex(0);
    m_calculationAreaEnd = (char *)(lastCalculationPointer.pointer());
    m_numberOfCalculations--;
    return;
  }
  char * calcI = (char *)calculationAtIndex(i).pointer();
  char * nextCalc = (char *) calculationAtIndex(i-1).pointer();
  assert(m_calculationAreaEnd >= nextCalc);
  size_t slidingSize = m_calculationAreaEnd - nextCalc;
  // Slide the i-1 most recent calculations right after the i+1'th
  memmove(calcI, nextCalc, slidingSize);
  m_calculationAreaEnd -= nextCalc - calcI;
  // Recompute pointer to calculations after the i'th
  recomputeMemoizedPointersAfterCalculationIndex(i);
  m_numberOfCalculations--;
}

// Delete the oldest calculation in the store and returns the amount of space freed by the operation
size_t CalculationStore::deleteOldestCalculation() {
  char * oldBufferEnd = (char *) m_calculationAreaEnd;
  deleteCalculationAtIndex(numberOfCalculations()-1);
  char * newBufferEnd = (char *) m_calculationAreaEnd;
  return oldBufferEnd - newBufferEnd;
}

// Delete all calculations
void CalculationStore::deleteAll() {
  m_calculationAreaEnd = m_buffer;
  m_numberOfCalculations = 0;
}

// Replace "Ans" by its expression
Expression CalculationStore::ansExpression(Context * context) {
  const Expression defaultAns = Rational::Builder(0);
  if (numberOfCalculations() == 0) {
    return defaultAns;
  }
  ExpiringPointer<Calculation> mostRecentCalculation = calculationAtIndex(0);
  Expression exactOutput = mostRecentCalculation->exactOutput();
  Expression input = mostRecentCalculation->input();
  if (exactOutput.isUninitialized() || input.isUninitialized()) {
    return defaultAns;
  }
  /* Special case: the exact output is a Store/Equal expression.
   * Store/Equal expression can only be at the root of an expression.
   * To avoid turning 'ans->A' in '2->A->A' or '2=A->A' (which cannot be
   * parsed), ans is replaced by the approximation output when any Store or
   * Equal expression appears. */
  bool exactOuptutInvolvesStoreEqual = exactOutput.type() == ExpressionNode::Type::Store || exactOutput.type() == ExpressionNode::Type::Equal;
  if (input.recursivelyMatches(Expression::IsApproximate, context) || exactOuptutInvolvesStoreEqual) {
    Expression approximate = mostRecentCalculation->approximateOutput(context, Calculation::NumberOfSignificantDigits::Maximal);
    if (approximate.isUninitialized()) {
      return defaultAns;
    }
    return approximate;
  }
  /* Special case: If exact output was hidden, it should not be accessible using
   * ans, unless it is equal to a non-undef and non-unreal approximation. */
  const char * exactOutputText = mostRecentCalculation->exactOutputText();
  const char * approximateOutputText = mostRecentCalculation->approximateOutputText(Calculation::NumberOfSignificantDigits::UserDefined);
  if (mostRecentCalculation->displayOutput(context) == Calculation::DisplayOutput::ApproximateOnly && (strcmp(approximateOutputText, exactOutputText) != 0 ||
      exactOutput.type() == ExpressionNode::Type::Unreal || exactOutput.type() == ExpressionNode::Type::Undefined)) {
    return input;
  }
  return exactOutput;
}

// Push converted expression in the buffer
bool CalculationStore::pushSerializedExpression(Expression e, char ** start, char * end, int numberOfSignificantDigits) {
  assert(end <= addressOfPointerToCalculationOfIndex(0) && *start < end && end - *start < m_bufferSize);
  while (true) {
    assert(*start >= m_buffer);
    int availableSize = end - *start;
    int serializationSize = PoincareHelpers::Serialize(e, *start, availableSize, numberOfSignificantDigits);
    if (serializationSize < availableSize - 1) {
      *start += serializationSize + 1;
      return true;
    }
    if (*start == m_buffer) {
      break;
    }
    *start -= deleteOldestCalculation();
  }
  return false;
}



Shared::ExpiringPointer<Calculation> CalculationStore::emptyStoreAndPushUndef(Context * context, HeightComputer heightComputer) {
  /* We end up here as a result of a failed calculation push. The store
   * attributes are not necessarily clean, so we need to reset them. */
  deleteAll();
  return push(Undefined::Name(), context, heightComputer);
}

// Recompute memoized pointers to the calculations after index i
void CalculationStore::recomputeMemoizedPointersAfterCalculationIndex(int index) {
  assert(index < m_numberOfCalculations);
  // Clear pointer and recompute new ones
  Calculation * c = calculationAtIndex(index).pointer();
  Calculation * nextCalc;
  while (index != 0) {
    nextCalc = c->next();
    memcpy(addressOfPointerToCalculationOfIndex(index), &nextCalc, sizeof(Calculation *));
    c = nextCalc;
    index--;
  }
}

}
