#ifndef SOLVER_EQUATION_STORE_H
#define SOLVER_EQUATION_STORE_H

#include <apps/shared/expression_model_store.h>
#include <poincare/symbol_abstract.h>

#include "equation.h"
#include "solution.h"

namespace Solver {

class EquationStore : public Shared::ExpressionModelStore {
 public:
  constexpr static int k_maxNumberOfEquations =
      Poincare::Expression::k_maxNumberOfVariables;
  constexpr static const char* k_extension = Ion::Storage::equationExtension;

  // ExpressionModelStore
  Ion::Storage::Record::ErrorStatus addEmptyModel() override;
  Shared::ExpiringPointer<Equation> modelForRecord(
      Ion::Storage::Record record) const {
    return Shared::ExpiringPointer<Equation>(
        static_cast<Equation*>(privateModelForRecord(record)));
  }
  static void RemoveAll() {
    Shared::ExpressionModelStore::RemoveAll(k_extension);
  }

 private:
  // ExpressionModelStore
  const char* modelExtension() const override { return k_extension; }
  int maxNumberOfMemoizedModels() const override {
    return k_maxNumberOfEquations;
  }
  Shared::ExpressionModelHandle* setMemoizedModelAtIndex(
      int cacheIndex, Ion::Storage::Record) const override;
  Shared::ExpressionModelHandle* memoizedModelAtIndex(
      int cacheIndex) const override;

  mutable Equation m_equations[k_maxNumberOfEquations];
};

}  // namespace Solver

#endif
