#ifndef STYLE_H
#define STYLE_H

#include <QColor>
#include <QJsonObject>
#include <QString>
#include <QStringView>
#include <QVector>

#include <memory>

#include "font.h"

namespace xi {

int utf8OffsetToUtf16(const QString &text, int ix);

class Style {
    friend class StyleMapState;

public:
    Style(const QColor &fgColor,
          const QColor &bgColor, bool underline, bool italic, int weight) : 
        m_fgColor(fgColor), m_bgColor(bgColor), m_underline(underline) {
        m_fontStyle.italic = italic;
        m_fontStyle.underline = underline;
        m_fontStyle.weight = weight;
    }

private:
    QColor m_fgColor;
    QColor m_bgColor;
    FontStyle m_fontStyle;
    bool m_fakeItalic = false;
    bool m_underline;
};

} // namespace xi

#endif // STYLE_H
