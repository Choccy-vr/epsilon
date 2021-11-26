#ifndef GRAPH_TEXT_FIELD_WITH_MAX_LENGTH_AND_EXTENSION_H
#define GRAPH_TEXT_FIELD_WITH_MAX_LENGTH_AND_EXTENSION_H

#include <apps/shared/text_field_with_extension.h>

namespace Graph {

class TextFieldWithMaxLengthAndExtension : public Shared::TextFieldWithExtension {
public:
  TextFieldWithMaxLengthAndExtension(size_t extensionLength,
      Escher::Responder * parentResponder,
      char * textBuffer,
      size_t textBufferSize,
      size_t draftTextBufferSize,
      Escher::InputEventHandlerDelegate * inputEventHandlerDelegate,
      Escher::TextFieldDelegate * delegate = nullptr,
      const KDFont * size = KDFont::LargeFont,
      float horizontalAlignment = KDContext::k_alignLeft,
      float verticalAlignment = KDContext::k_alignCenter,
      KDColor textColor = KDColorBlack,
      KDColor backgroundColor = KDColorWhite) :
    TextFieldWithExtension(extensionLength, parentResponder, textBuffer, textBufferSize, draftTextBufferSize, inputEventHandlerDelegate, delegate, size, horizontalAlignment, verticalAlignment, textColor, backgroundColor) {}
  void setDraftTextBufferSize(size_t size) { m_contentView.setDraftTextBufferSize(size); }
};

}

#endif
