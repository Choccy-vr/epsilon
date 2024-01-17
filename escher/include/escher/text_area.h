#ifndef ESCHER_TEXT_AREA_H
#define ESCHER_TEXT_AREA_H

#include <assert.h>
#include <escher/text_input.h>
#include <omg/directions.h>
#include <string.h>

#include <array>

namespace Escher {

class TextArea : public TextInput {
 public:
  constexpr static char k_indentation[] = "  ";
  constexpr static int k_indentationSpaces = std::size(k_indentation) - 1;
  constexpr static char k_newLine[] = "\n";

  TextArea(Responder* parentResponder, View* contentView);
  bool handleEvent(Ion::Events::Event event) override;
  bool handleEventWithText(const char* text, bool indentation = false,
                           bool forceCursorRightOfText = false) override;
  void setText(char* textBuffer, size_t textBufferSize);

 protected:
  int indentationBeforeCursor() const;

  class Text {
   public:
    Text(char* buffer, size_t bufferSize)
        : m_buffer(buffer), m_bufferSize(bufferSize) {}
    void setText(char* buffer, size_t bufferSize) {
      m_buffer = buffer;
      m_bufferSize = bufferSize;
    }
    const char* text() const { return m_buffer; }

    class Line {
     public:
      Line(const char* text);
      const char* text() const { return m_text; }
      size_t charLength() const { return m_charLength; }
      KDCoordinate glyphWidth(KDFont::Size const font) const;
      bool contains(const char* c) const;

     private:
      const char* m_text;
      size_t m_charLength;
    };

    class LineIterator {
     public:
      LineIterator(const char* text) : m_line(text) {}
      Line operator*() { return m_line; }
      LineIterator& operator++();
      bool operator!=(const LineIterator& it) const {
        return m_line.text() != it.m_line.text();
      }

     private:
      Line m_line;
    };

    class Position {
      /* column and line correspond to the visual column and line. The glyph at
       * the kth column is not the the glyph of kth code point, because of
       * combining code points that do not fave a personnal glyph. */
     public:
      Position(int column, int line) : m_column(column), m_line(line) {}
      int column() const { return m_column; }
      int line() const { return m_line; }

     private:
      int m_column;
      int m_line;
    };

    LineIterator begin() const { return LineIterator(m_buffer); };
    LineIterator end() const { return LineIterator(nullptr); };

    KDSize span(KDFont::Size const font) const;

    Position positionAtPointer(const char* pointer) const;
    const char* pointerAtPosition(Position p);

    void insertText(const char* s, int textLength, char* location);
    void insertSpacesAtLocation(int numberOfSpaces, char* location);

    CodePoint removePreviousGlyph(char** position);
    size_t removeText(const char* start, const char* end);
    size_t removeRemainingLine(const char* position,
                               OMG::HorizontalDirection direction);
    char operator[](size_t index) {
      assert(index < m_bufferSize);
      return m_buffer[index];
    }
    size_t bufferSize() const { return m_bufferSize; }
    size_t textLength() const { return strlen(m_buffer); }
    int textLineTotal() const {
      return positionAtPointer(m_buffer + textLength()).line();
    }

   private:
    char* m_buffer;
    size_t m_bufferSize;
  };

  class ContentView : public TextInput::ContentView {
   public:
    ContentView(KDFont::Size font)
        : TextInput::ContentView({.style = {.font = font},
                                  .horizontalAlignment = KDGlyph::k_alignLeft,
                                  .verticalAlignment = KDGlyph::k_alignCenter}),
          m_text(nullptr, 0) {
      m_cursorLocation = m_text.text();
    }
    void drawRect(KDContext* ctx, KDRect rect) const override;
    void drawStringAt(KDContext* ctx, int line, int column, const char* text,
                      int length, KDColor textColor, KDColor backgroundColor,
                      const char* selectionStart, const char* selectionEnd,
                      KDColor backgroundHighlightColor) const;
    virtual void drawLine(KDContext* ctx, int line, const char* text,
                          size_t length, int fromColumn, int toColumn,
                          const char* selectionStart,
                          const char* selectionEnd) const = 0;
    virtual void clearRect(KDContext* ctx, KDRect rect) const = 0;
    KDSize minimalSizeForOptimalDisplay() const override;
    void setText(char* textBuffer, size_t textBufferSize);
    const char* text() const override { return m_text.text(); }
    const char* draftText() const override { return m_text.text(); }
    const Text* getText() const { return &m_text; }
    bool insertTextAtLocation(const char* text, char* location,
                              int textLength = -1) override;
    void moveCursorGeo(int deltaX, int deltaY);
    bool removePreviousGlyph() override;
    bool removeEndOfLine() override;
    bool removeStartOfLine();
    size_t removeText(const char* start, const char* end);
    size_t deleteSelection() override;

   protected:
    KDRect glyphFrameAtPosition(const char* text,
                                const char* position) const override;
    Text m_text;
  };

  ContentView* contentView() {
    return static_cast<ContentView*>(TextInput::contentView());
  }

 private:
  void selectUpDown(OMG::VerticalDirection direction, int step);

  /* KDCoordinates are 16 bits wide. It limits the number of lines
   * along with the length of lines:
   * k_maxLines < (2^15-1 - padding) / glyphHeight(big font) = 1800
   * k_maxLineChars < (2^15-1 - padding) / glyphWidth(big font) = 3200
   * We have chosen arbitrary values that respect this constraint. */
  constexpr static int k_maxLines = 999;
  constexpr static int k_maxLineChars = 3000;

  static_assert(k_maxLines * KDFont::GlyphHeight(KDFont::Size::Large) <
                KDCOORDINATE_MAX);
  static_assert(k_maxLineChars * KDFont::GlyphWidth(KDFont::Size::Large) <
                KDCOORDINATE_MAX);
};

}  // namespace Escher
#endif
