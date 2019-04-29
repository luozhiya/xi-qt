#include "config.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

namespace xi {

static constexpr const char *names[] = {
    "auto_indent",
    "font_face",
    "font_size",
    "line_ending",
    "plugin_search_path",
    "scroll_past_end",
    "tab_size",
    "translate_tabs_to_spaces",
    "use_tab_stops",
    "word_wrap",
    "wrap_width"
};

QVariant to_variant(const QJsonValue &json) {
    if (json.isBool()) {
        return QVariant(json.toBool());
    } else if (json.isString()) {
        return QVariant(json.toString());
    } else if (json.isArray()) {
        QStringList ss;
        QJsonArray array = json.toArray();
        foreach (const QJsonValue &v, array) {
            ss.push_back(v.toString());
        }
        return QVariant(ss);
    } else if (json.isDouble()) {
        return QVariant(json.toDouble());
    }
    return QVariant(QVariant::Invalid);
}

void ConfigState::applyUpdate(const QJsonObject &json) {
    for (auto i = 0; i < std::size(names); ++i) {
        const QString name = names[i];
        auto &element = m_elements[name];
        element = to_variant(json[name]);
    }
}

} // namespace xi