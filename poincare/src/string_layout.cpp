#include <poincare/string_layout.h>
#include <algorithm>
#include <ion/unicode/utf8_helper.h>
#include <escher/metric.h>
#include <poincare/layout_helper.h>

namespace Poincare {

StringLayoutNode::StringLayoutNode(const char * string, int stringSize, const KDFont * font) :
  LayoutNode(),
  StringFormat(font),
  m_decimalOrInteger(false)
  {
    m_stringLength = strlcpy(m_string, string, stringSize);
  }

Layout StringLayoutNode::makeEditable() {
  return StringLayout(this).makeEditable();
}

int StringLayoutNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return strlcpy(buffer, m_string, bufferSize);
}

size_t StringLayoutNode::size() const {
  return sizeof(StringLayoutNode) + sizeof(char) * (m_stringLength + 1);
}

bool StringLayoutNode::protectedIsIdenticalTo(Layout l) {
  assert(l.type() == Type::StringLayout);
  StringLayout & sl = static_cast<StringLayout &>(l);
  return strncmp(m_string, sl.string(), std::max(m_stringLength + 1, sl.stringLength() + 1)) == 0;
}

// Sizing and positioning
KDSize StringLayoutNode::computeSize() {
  KDSize glyph = font()->glyphSize();
  return KDSize(UTF8Helper::StringGlyphLength(m_string) * glyph.width() + numberOfThousandsSeparators() * Escher::Metric::ThousandsSeparatorWidth, glyph.height());
}

KDCoordinate StringLayoutNode::computeBaseline() {
  return font()->glyphSize().height() / 2;
}

void StringLayoutNode::render(KDContext * ctx, KDPoint p, KDColor expressionColor, KDColor backgroundColor, Layout * selectionStart, Layout * selectionEnd, KDColor selectionColor) {
  int nThousandsSeparators = numberOfThousandsSeparators();
  if (nThousandsSeparators == 0) {
    ctx->drawString(m_string, p, font(), expressionColor, backgroundColor);
    return;
  }
  // Draw the thousand separators
  int firstSeparatorIndex = firstNonDigitIndex() - 3 * nThousandsSeparators - 1;
  // Use this buffer to draw group of 3 digits.
  char groupedNumbersBuffer[4];
  // Draw the first separator first
  strlcpy(groupedNumbersBuffer, m_string, firstSeparatorIndex + 2);
  p = ctx->drawString(groupedNumbersBuffer, p, font(), expressionColor, backgroundColor);
  p = p.translatedBy(KDPoint(Escher::Metric::ThousandsSeparatorWidth, 0));
  // Draw the other separators.
  for (int i = 0; i < nThousandsSeparators - 1; i++) {
    strlcpy(groupedNumbersBuffer, &m_string[firstSeparatorIndex + i * 3 + 1], 4);
    p = ctx->drawString(groupedNumbersBuffer, p, font(), expressionColor, backgroundColor);
    p = p.translatedBy(KDPoint(Escher::Metric::ThousandsSeparatorWidth, 0));
  }
  // Draw the end of the string.
  ctx->drawString(&m_string[firstSeparatorIndex + 3 * (nThousandsSeparators - 1) + 1], p, font(), expressionColor, backgroundColor);
}

int StringLayoutNode::numberOfThousandsSeparators() {
  if (!m_decimalOrInteger) {
    return 0;
  }
  int nonDigitIndex = firstNonDigitIndex();
  bool isNegative = m_string[0] == '-';
  if (nonDigitIndex - isNegative < k_minDigitsForThousandSeparator) {
    return 0;
  }
  return (nonDigitIndex - isNegative - 1) / 3;
}

int StringLayoutNode::firstNonDigitIndex() {
  if (!m_decimalOrInteger) {
    return -1;
  }
  int nonDigitIndex = m_string[0] == '-';
  while (nonDigitIndex < m_stringLength) {
    if (!('0' <= m_string[nonDigitIndex] && '9' >=  m_string[nonDigitIndex])) {
      break;
    }
    nonDigitIndex++;
  }
  return nonDigitIndex;
}

StringLayout StringLayout::Builder(const char * string, int stringSize, const KDFont * font) {
  void * bufferNode = TreePool::sharedPool()->alloc(sizeof(StringLayoutNode) + sizeof(char) * stringSize);
  StringLayoutNode * node = new (bufferNode) StringLayoutNode(string, stringSize, font);
  TreeHandle h = TreeHandle::BuildWithGhostChildren(node);
  return static_cast<StringLayout &>(h);
}

Layout StringLayout::makeEditable() {
  Layout editableLayout = LayoutHelper::StringToCodePointsLayout(string(), stringLength(), font());
  replaceWithInPlace(editableLayout);
  return editableLayout;
}

}
