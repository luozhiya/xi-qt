#include "style_span.h"
#include "style.h"

#include <QDebug>

namespace xi {

StyleSpan::StyleSpan(StyleIdentifier style, RangeI range) : m_style(style), m_range(range) {
}

StyleSpan::StyleSpan() : m_style(-1) {
}

std::shared_ptr<QList<StyleSpan>> StyleSpan::styles(const QJsonArray &json, const QString &text) {
    auto vss = std::make_shared<QList<StyleSpan>>();
    auto ix = 0;
    for (auto i = 0; i < json.size(); i += 3) {
        auto start = ix + json.at(i).toInt();
        auto end = start + json.at(i + 1).toInt();
        auto style = json.at(i + 2).toInt();
        auto startIx = utf8OffsetToUtf16(text, start);
        auto endIx = utf8OffsetToUtf16(text, end);
        if (startIx < 0 || endIx < startIx) {
            qWarning() << "malformed style array for line: " << text << json;
        } else {
            vss->append(StyleSpan(style, RangeI(startIx, endIx))); //
        }
        ix = end;
    }
    return vss;
}

} // namespace xi