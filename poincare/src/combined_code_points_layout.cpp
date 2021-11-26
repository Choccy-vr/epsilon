#include <poincare/combined_code_points_layout.h>
#include <poincare/layout_helper.h>
#include <poincare/serialization_helper.h>
#include <escher/metric.h>

namespace Poincare {

// LayoutNode

int CombinedCodePointsLayoutNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  int numberOfChar = SerializationHelper::CodePoint(buffer, bufferSize, m_codePoint);
  numberOfChar += SerializationHelper::CodePoint(buffer + numberOfChar, bufferSize - numberOfChar, m_CombinedCodePoints);
  return numberOfChar;
}

void CombinedCodePointsLayoutNode::render(KDContext * ctx, KDPoint p, KDColor expressionColor, KDColor backgroundColor, Layout * selectionStart, Layout * selectionEnd, KDColor selectionColor) {
  assert(m_displayType == DisplayType::None);
  constexpr int bufferSize =  2 * sizeof(CodePoint)/sizeof(char) + 1; // Null-terminating char
  char buffer[bufferSize];
  serialize(buffer, bufferSize, Preferences::PrintFloatMode::Decimal, 0);
  ctx->drawString(buffer, p, m_font, expressionColor, backgroundColor);
}

bool CombinedCodePointsLayoutNode::protectedIsIdenticalTo(Layout l) {
  assert(l.type() == Type::CombinedCodePointsLayout);
  CombinedCodePointsLayout & cpl = static_cast<CombinedCodePointsLayout &>(l);
  return CombinedCodePoints() == cpl.CombinedCodePoints() && CodePointLayoutNode::protectedIsIdenticalTo(l);
}

CombinedCodePointsLayout CombinedCodePointsLayout::Builder(CodePoint mainCodePoint, CodePoint CombinedCodePoints, const KDFont * font) {
  void * bufferNode = TreePool::sharedPool()->alloc(sizeof(CombinedCodePointsLayoutNode));
  CombinedCodePointsLayoutNode * node = new (bufferNode) CombinedCodePointsLayoutNode(mainCodePoint, CombinedCodePoints, font);
  TreeHandle h = TreeHandle::BuildWithGhostChildren(node);
  return static_cast<CombinedCodePointsLayout &>(h);
}

}
