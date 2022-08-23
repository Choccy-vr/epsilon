#include "record_delegate.h"

namespace Shared {

size_t RecordDelegate::precedenceScoreOfExtension(const char * extension) {
  for (int i = 0 ; i < k_numberOfRestrictiveExtensions ; i++) {
    if (strcmp(extension, k_restrictiveExtensions[i]) == 0) {
      return k_restrictiveExtensionsPrecedenceScore[i];
    }
  }
  return -1;
}

Ion::RecordDelegate::OverrideStatus RecordDelegate::shouldRecordBeOverridenWithNewExtension(Ion::Storage::Record previousRecord, const char * newExtension) {
  if (previousRecord.isNull()) {
    return Ion::RecordDelegate::OverrideStatus::Allowed;
  }
  assert(!previousRecord.hasExtension(newExtension));
  int newPrecedenceScore = precedenceScoreOfExtension(newExtension);
  int previousPrecedenceScore = precedenceScoreOfExtension(previousRecord.name().extension);
  // If at least one is not a restrictive extension, they can coexist.
  if (newPrecedenceScore == -1 || previousPrecedenceScore == -1) {
    return Ion::RecordDelegate::OverrideStatus::CanCoexist;
  }
  Ion::Storage::Record::Name previousName = previousRecord.name();
  bool newIsReservedForAnotherExtension = isNameReservedForAnotherExtension(previousName.baseName, previousName.baseNameLength, newExtension);
  bool previousIsReservedForAnotherExtension = isNameReservedForAnotherExtension(previousName.baseName, previousName.baseNameLength, previousRecord.name().extension);
  if ((newIsReservedForAnotherExtension && !previousIsReservedForAnotherExtension) // since they have the same baseName, it means that previousRecord is reserved for its extension.
      || newPrecedenceScore > previousPrecedenceScore) {
    return Ion::RecordDelegate::OverrideStatus::Forbidden;
  }
  return  Ion::RecordDelegate::OverrideStatus::Allowed;
}

bool RecordDelegate::isNameReservedForAnotherExtension(const char * name, int nameLength, const char * extension) {
  for (int i = 0 ; i < k_reservedExtensionsLength ; i++) {
    ReservedExtension reservedExtension = k_reservedExtensions[i];
    /* If it it has the same extension, skip the test.
     * We only search if the name is reserved for an OTHER extension.
     * */
    if (strcmp(extension, reservedExtension.extension) == 0) {
      continue;
    }
    for (int j = 0 ; j < reservedExtension.numberOfElements ; j++) {
      // Check if the record has the same base name.
      bool hasSuffixDigit = reservedExtension.prefixRepetitions > 0;
      // First check the prefix:
      int diffBetweenPrefix = strncmp(name, reservedExtension.namePrefixes[j], nameLength - static_cast<int>(hasSuffixDigit));
      // Then check suffix if needed:
      if (diffBetweenPrefix == 0
          && (!hasSuffixDigit
            || (hasSuffixDigit
              && name[nameLength - 1] >= '1'
              && name[nameLength - 1] < '1' + reservedExtension.prefixRepetitions
              )
            )
          ) {
        return true;
      }
    }
  }
  return false;
}

}
