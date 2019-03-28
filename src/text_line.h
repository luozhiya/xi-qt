#ifndef TEXT_LINE_H
#define TEXT_LINE_H

#include <QFontMetrics>
#include <QFontMetricsF>
#include <QObject>
#include <QPainter>
#include <QTextCharFormat>
#include <QTextLayout>

#include <memory>

#include "font.h"
#include "range.h"
#include "style_span.h"

namespace xi {

struct SelRange {
    QColor color;
    RangeI range;
};

struct BackgroundColorRange {
    RangeF range;
    QColor color;
};

struct UnderlineRange {
    RangeF range;
    RangeF y;
    QColor color;
};

template <typename T>
struct Span {
    Span(const RangeI &range, const T &payload) : range(range), payload(payload) {
        //this->range = range;
        //this->payload = payload;
    }
    Span(const Span<T> &span) {
        *this = span;
    }
    Span &operator=(const Span<T> &span) {
        if (this != &span) {
            range = span.range;
            payload = span.payload;
        }
        return *this;
    }
    RangeI range;
    T payload;
};

struct Empty {
};

enum class UnderlineStyle {
    single,
    thick
};

using ColorSpan = Span<QColor>;
using UnderlineSpan = Span<UnderlineStyle>;
using SimpleSpan = Span<Empty>;
using FontSpan = Span<FontStyle>;

// Render info, line
class TextLine {
    friend class TextLineBuilder;

public:
    explicit TextLine(const QString &text, std::shared_ptr<Font> font);

    int xToIndex(qreal x);
    qreal indexTox(int ix);

    qreal width() const {
        return m_width;
    }
    std::shared_ptr<QTextLayout> layout() const {
        return m_layout;
    }
    std::shared_ptr<QFontMetricsF> metrics() const {
        return m_fontMetrics;
    }
    std::shared_ptr<QList<SelRange>> selRanges() const {
        return m_selRanges;
    }

protected:
    std::shared_ptr<Font> m_font;
    QString m_text;
    qreal m_width;

    std::shared_ptr<QList<StyleSpan>> m_styles;
    std::shared_ptr<QFontMetricsF> m_fontMetrics;
    std::shared_ptr<QTextLayout> m_layout;
    std::shared_ptr<QList<SelRange>> m_selRanges;
};

class TextLineBuilder {
public:
    TextLineBuilder(const QString &text, std::shared_ptr<Font> font) {
        m_text = text;
        m_font = font;
    }

    void setFgColor(const QColor &color) {
        m_defaultFgColor = color;
    }

    void addFontSpan(const RangeI &range, const FontStyle &info) {
        if (!range.isEmpty()) {
            m_fontSpans.append(std::make_shared<FontSpan>(range, info));
        }
    }

    void addFgSpan(const RangeI &range, const QColor &color) {
        if (!range.isEmpty()) {
            m_fgSpans.append(std::make_shared<ColorSpan>(range, color));
        }
    }

    void addSelSpan(const RangeI &range, const QColor &color) {
        if (!range.isEmpty()) {
            m_selSpans.append(std::make_shared<ColorSpan>(range, color));
        }
    }

    void addFakeItalicSpan(const RangeI &range) {
        if (!range.isEmpty()) {            
            m_fakeItalicSpans.append(std::make_shared<SimpleSpan>(range, Empty()));
        }
    }

    void addUnderlineSpan(const RangeI &range, UnderlineStyle style) {
        if (!range.isEmpty()) {
            m_underlineSpans.append(std::make_shared<UnderlineSpan>(range, style));
        }
    }

    // init QTextLayout
    std::shared_ptr<TextLine> build();

private:
    QVector<QTextLayout::FormatRange> m_overrides;
    
    std::shared_ptr<Font> m_font;
    QString m_text;
    QColor m_defaultFgColor;

    // TODO: MULTI FONTS
    //QList<std::shared_ptr<Font>> m_fonts;

    QList<std::shared_ptr<FontSpan>> m_fontSpans;
    QList<std::shared_ptr<ColorSpan>> m_fgSpans;
    QList<std::shared_ptr<ColorSpan>> m_selSpans;
    QList<std::shared_ptr<SimpleSpan>> m_fakeItalicSpans;
    QList<std::shared_ptr<UnderlineSpan>> m_underlineSpans;
};

class Painter {
public:
    //static void drawLineBg(QPainter &painter, const std::shared_ptr<TextLine> &line, qreal x, const RangeF &y);

    static void drawLine(QPainter &painter, std::shared_ptr<TextLine> line, qreal x, qreal y) {
        line->layout()->draw(&painter, QPoint(x, y));
    }

    //static void drawLineDecorations(QPainter &painter, const std::shared_ptr<TextLine> &line, qreal x, qreal y) {
    //}

    static void drawCursor(QPainter &painter, qreal x, qreal y, qreal width, qreal height, const QColor &fg) {
        painter.fillRect(x, y, width, height, fg);
    }
};

} // namespace xi

#endif // TEXT_LINE_H
