#ifndef APPS_SHARED_GLOBAL_CONTEXT_H
#define APPS_SHARED_GLOBAL_CONTEXT_H

#include <assert.h>
#include <ion/storage/file_system.h>
#include <omg/global_box.h>
#include <poincare/context.h>
#include <poincare/decimal.h>
#include <poincare/float.h>
#include <poincare/matrix.h>
#include <poincare/symbol.h>

#include <array>

#include "continuous_function_store.h"
#include "sequence_context.h"
#include "sequence_store.h"

namespace Shared {

class GlobalContext final : public Poincare::Context {
 public:
  constexpr static const char *k_extensions[] = {
      Ion::Storage::expExtension,  Ion::Storage::matExtension,
      Ion::Storage::funcExtension, Ion::Storage::lisExtension,
      Ion::Storage::seqExtension,  Ion::Storage::regExtension,
      Ion::Storage::pcExtension};
  constexpr static int k_numberOfExtensions = std::size(k_extensions);

  // Storage information
  static bool SymbolAbstractNameIsFree(const char *baseName);

  static const Poincare::Layout LayoutForRecord(Ion::Storage::Record record);

  // Destroy records
  static void DestroyRecordsBaseNamedWithoutExtension(const char *baseName,
                                                      const char *extension);

  GlobalContext() : m_sequenceContext(this, sequenceStore){};
  /* Expression for symbol
   * The expression recorded in global context is already an expression.
   * Otherwise, we would need the context and the angle unit to evaluate it */
  SymbolAbstractType expressionTypeForIdentifier(const char *identifier,
                                                 int length) override;
  bool setExpressionForSymbolAbstract(
      const Poincare::Expression &expression,
      const Poincare::SymbolAbstract &symbol) override;
  static OMG::GlobalBox<SequenceStore> sequenceStore;
  static OMG::GlobalBox<ContinuousFunctionStore> continuousFunctionStore;
  void storageDidChangeForRecord(const Ion::Storage::Record record);
  SequenceContext *sequenceContext() { return &m_sequenceContext; }
  void tidyDownstreamPoolFrom(
      Poincare::TreeNode *treePoolCursor = nullptr) override;
  void prepareForNewApp();
  void reset();

 private:
  // Expression getters
  const Poincare::Expression protectedExpressionForSymbolAbstract(
      const Poincare::SymbolAbstract &symbol, bool clone,
      Poincare::ContextWithParent *lastDescendantContext) override;
  const Poincare::Expression expressionForSymbolAndRecord(
      const Poincare::SymbolAbstract &symbol, Ion::Storage::Record r,
      Context *ctx);
  static const Poincare::Expression ExpressionForActualSymbol(
      Ion::Storage::Record r);
  static const Poincare::Expression ExpressionForFunction(
      const Poincare::Expression &parameter, Ion::Storage::Record r);
  const Poincare::Expression expressionForSequence(
      const Poincare::SymbolAbstract &symbol, Ion::Storage::Record r,
      Context *ctx);
  // Expression setters
  /* This modifies the expression. */
  Ion::Storage::Record::ErrorStatus setExpressionForActualSymbol(
      Poincare::Expression &expression, const Poincare::SymbolAbstract &symbol,
      Ion::Storage::Record previousRecord);
  Ion::Storage::Record::ErrorStatus setExpressionForFunction(
      const Poincare::Expression &expression,
      const Poincare::SymbolAbstract &symbol,
      Ion::Storage::Record previousRecord);
  // Record getter
  static Ion::Storage::Record SymbolAbstractRecordWithBaseName(
      const char *name);
  SequenceContext m_sequenceContext;
};

}  // namespace Shared

#endif
