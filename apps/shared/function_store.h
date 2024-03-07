#ifndef SHARED_FUNCTION_STORE_H
#define SHARED_FUNCTION_STORE_H

#include <stdint.h>

#include "expression_model_store.h"
#include "function.h"

namespace Shared {

// FunctionStore stores functions and gives them a color.

class FunctionStore : public ExpressionModelStore {
 public:
  FunctionStore() : ExpressionModelStore() {}
  virtual int numberOfActiveFunctions() const {
    return numberOfModelsSatisfyingTest(&IsFunctionActive, nullptr);
  }
  Ion::Storage::Record activeRecordAtIndex(int i) const {
    return recordSatisfyingTestAtIndex(i, &IsFunctionActive, nullptr);
  }
  int indexOfRecordAmongActiveRecords(Ion::Storage::Record record) const {
    int n = numberOfActiveFunctions();
    for (int i = 0; i < n; i++) {
      if (activeRecordAtIndex(i) == record) {
        return i;
      }
    }
    return -1;
  }
  ExpiringPointer<Function> modelForRecord(Ion::Storage::Record record) const {
    return ExpiringPointer<Function>(
        static_cast<Function *>(privateModelForRecord(record)));
  }
  virtual KDColor colorForRecord(Ion::Storage::Record record,
                                 int subCurveIndex) const = 0;

 protected:
  static bool IsFunctionActive(ExpressionModelHandle *model, void *context) {
    // An active function must be defined
    return isModelDefined(model, context) &&
           static_cast<Function *>(model)->isActive();
  }
};

}  // namespace Shared

#endif
