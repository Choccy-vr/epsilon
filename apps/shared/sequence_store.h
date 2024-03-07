#ifndef APPS_SHARED_SEQUENCE_STORE_H
#define APPS_SHARED_SEQUENCE_STORE_H

#include <stdint.h>

#include "function_store.h"
#include "sequence.h"

namespace Shared {

class SequenceStore : public FunctionStore {
 public:
  using FunctionStore::FunctionStore;
  /* Sequence Store hold all its Sequences in an array. The Sequence pointers
   * return by modelForRecord are therefore non-expirable. We choose to return
   * Sequence * instead of ExpiringPointer<Sequence>. */
  Sequence *modelForRecord(Ion::Storage::Record record) const {
    return static_cast<Sequence *>(privateModelForRecord(record));
  }
  Ion::Storage::Record::ErrorStatus addEmptyModel() override;
  /* WARNING: after calling removeModel or removeAll, the sequence context
   * need to invalidate its cache as the sequences evaluations might have
   * changed */
  /* If the sequences have been defined in a unusual order, recordAtIndex(0)
   * may not be the record for u. */
  Ion::Storage::Record recordAtNameIndex(int i) const {
    return Ion::Storage::FileSystem::sharedFileSystem
        ->recordBaseNamedWithExtension(k_sequenceNames[i], modelExtension());
  }

  static int SequenceIndexForName(char name);
  static const char *FirstAvailableName(size_t *nameIndex = nullptr);
  constexpr static const char *k_sequenceNames[] = {"u", "v", "w"};
  constexpr static size_t k_maxSequenceNameLength = []() {
    size_t m = 0;
    for (const char *s : k_sequenceNames) {
      m = std::max<size_t>(m, Poincare::Helpers::StringLength(s));
    }
    return m;
  }();
  constexpr static int k_maxNumberOfSequences = 3;
  constexpr static int k_maxRecurrenceDepth = 2;
  Sequence sequenceAtIndex(int i) {
    assert(i < SequenceStore::k_maxNumberOfSequences && i >= 0);
    return m_sequences[i];
  }
  float smallestInitialRank() const;

 private:
  int maxNumberOfMemoizedModels() const override {
    return SequenceStore::k_maxNumberOfSequences;
  }
  const char *modelExtension() const override {
    return Ion::Storage::sequenceExtension;
  }
  /* We don't use model memoization for two reasons:
   * - the number of Sequence is capped so we keep enough Sequences to store
   * them all.
   * - evaluating sequences require to evaluate all of them at the same time
   *   (as they might be co-dependent) which doesn't suit our ring memoization.
   * To still make it work with the ExpressionModelStore, we force
   * setMemoizedModelAtIndex to store u, v and w sequences in this order in
   * m_sequences whatever the value of cacheIndex. We thus return a
   * ExpressionModelHandle pointer after setting the model as it won't be the
   * memoized model at cacheIndex. */
  ExpressionModelHandle *setMemoizedModelAtIndex(
      int cacheIndex, Ion::Storage::Record record) const override;
  ExpressionModelHandle *memoizedModelAtIndex(int cacheIndex) const override;
  mutable Sequence m_sequences[k_maxNumberOfSequences];
};

}  // namespace Shared

#endif
