#include "style_map.h"

#include <QJsonArray>
#include <QFontMetricsF>

#include "perference.h"

namespace xi {

QColor colorFromArgb(quint32 argb) {
    return QColor::fromRgbF(
        qreal((argb >> 16) & 0xff) * 1.0 / 255,
        qreal((argb >> 8) & 0xff) * 1.0 / 255,
        qreal(argb & 0xff) * 1.0 / 255,
        qreal((argb >> 24) & 0xff) * 1.0 / 255);
}

void StyleMapState::defStyle(const QJsonObject &json) {
    auto theme = Perference::shared()->theme()->locked();
    QColor fgColor(QColor::Invalid);
    QColor bgColor(QColor::Invalid);

    auto styleId = json["id"].toInt();
    if (json.contains("fg_color")) {
        fgColor = colorFromArgb(json["fg_color"].toVariant().toULongLong());
    } else {
        fgColor = theme->foreground();
    }

    if (json.contains("bg_color")) {
        bgColor = colorFromArgb(json["bg_color"].toInt());
    }

    auto underline = false;
    auto italic = false;
    auto weight = int(QFont::Normal);

    if (json.contains("underline")) {
        underline = json["underline"].toBool();
    }
    if (json.contains("italic")) {
        italic = json["italic"].toBool();
    }
    if (json.contains("weight")) {
        auto w = json["weight"].toInt();
        // 0 - 1000 [400]
        // 0 - 100 [50]
        weight = QFont::Bold;
    }

    auto style = std::make_shared<Style>(fgColor, bgColor, underline, italic, weight);

    while (m_styles.count() < styleId) {
        m_styles.append(nullptr);
    }
    if (m_styles.count() == styleId) {
        m_styles.append(style);
    } else {
        m_styles[styleId] = style;
    }
}

void StyleMapState::applyStyle(std::shared_ptr<TextLineBuilder> builder, int id, const RangeI &range, const QColor &selColor) {
    if (id > m_styles.count()) {
        qWarning() << "stylemap can't resolve" << id;
        return;
    }
    if (id == 0 || id == 1) {
        builder->addSelSpan(range, selColor);
    } else {
        auto style = m_styles[id];
        if (!style) return;

        if (style->m_fgColor.isValid()) {
            builder->addFgSpan(range, style->m_fgColor);
        }
        builder->addFontSpan(range, style->m_fontStyle);

        if (style->m_fakeItalic) {
            builder->addFakeItalicSpan(range);
        }
        if (style->m_underline) {
            builder->addUnderlineSpan(range, UnderlineStyle::single);
        }
    }
}

void StyleMapState::applyStyles(std::shared_ptr<TextLineBuilder> builder, std::shared_ptr<QList<StyleSpan>> styles, const QColor &selColor, const QColor &highlightColor) {
    foreach (StyleSpan ss, *styles) {
        QColor color;
        auto id = ss.style();
        switch (id) {
        case 0:
            color = selColor;
            break;
        case 1:
            color = highlightColor;
            break;
        default:
            color = QColor(QColor::Invalid);
            break;
        }
        applyStyle(builder, id, ss.range(), color);
    }
}

} // namespace xi
