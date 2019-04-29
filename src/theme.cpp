#include "theme.h"

#include <QPalette>

#include <iterator>

namespace xi {

// https://docs.rs/syntect/2.1.0/syntect/highlighting/struct.ThemeSettings.html
static constexpr const char *names[] = {
    "accent",                        // A color made available for use by the theme.
    "active_guide",                  // Color of the guide lined up with the caret. Only applied if the indent_guide_options setting is set to draw_active.
    "background",                    // The default backgound color of the view.
    "bracket_contents_foreground",   // Color of bracketed sections of text when the caret is in a bracketed section. Only applied when the match_brackets setting is set to true.
    "bracket_contents_options",      // Controls certain options when the caret is in a bracket section. Only applied when the match_brackets setting is set to true.
    "brackets_background",           // Background color of the brackets when the caret is next to a bracket. Only applied when the match_brackets setting is set to true.
    "brackets_foreground",           // Foreground color of the brackets when the caret is next to a bracket. Only applied when the match_brackets setting is set to true.
    "brackets_options",              // Controls certain options when the caret is next to a bracket. Only applied when the match_brackets setting is set to true.
    "caret",                         // Color of the caret.
    "find_highlight",                // Background color of regions matching the current search.
    "find_highlight_foreground",     // Text color of regions matching the current search.
    "foreground",                    // The default color for text.
    "guide",                         // Color of the guides displayed to indicate nesting levels.
    "gutter",                        // Background color of the gutter.
    "gutter_foreground",             // Foreground color of the gutter.
    "highlight",                     // The border color for "other" matches.
    "highlight_foreground",          // Deprecated!
    "inactive_selection",            // The background color of a selection in a view that is not currently focused.
    "inactive_selection_foreground", // A color that will override the scope-based text color of the selection in a view that is not currently focused.
    "line_highlight",                // Color of the line the caret is in. Only used when the higlight_line setting is set to true.
    "minimap_border",                // The color of the border drawn around the viewport area of the minimap. Only used when the draw_minimap_border setting is enabled.
    "misspelling",                   // The color to use for the squiggly underline drawn under misspelled words.
    "phantom_css",                   // CSS passed to phantoms.
    "popup_css",                     // CSS passed to popups.
    "selection",                     // The background color of selected text.
    "selection_background",          // Deprecated!
    "selection_border",              // Color of the selection regions border.
    "selection_foreground",          // A color that will override the scope-based text color of the selection.
    "shadow",                        // The color of the shadow used when a text area can be horizontally scrolled.
    "stack_guide",                   // Color of the current guide¡¯s parent guide level. Only used if the indent_guide_options setting is set to draw_active.
    "tags_foreground",               // Color of tags when the caret is next to a tag. Only used when the match_tags setting is set to true.
    "tags_options",                  // Controls certain options when the caret is next to a tag. Only applied when the match_tags setting is set to true.
};

ThemeState::Elements defaultThemeElements() {
    ThemeState::Elements elements;
    elements["foreground"] = QColor::fromRgb(255, 215, 0);
    elements["background"] = Qt::black;
    elements["caret"] = QColor::fromRgb(220, 220, 220);
    return elements;
}

QColor to_color(const QJsonObject &json) {
    auto a = json["a"].toInt();
    auto b = json["b"].toInt();
    auto g = json["g"].toInt();
    auto r = json["r"].toInt();
    return QColor(r, g, b, a);
}

 ThemeState::ThemeState() {
}

void ThemeState::applyUpdate(const QString &name, const QJsonObject &json) {
    m_name = name;
    for (auto i = 0; i < std::size(names); ++i) {
        QString name = names[i];
        auto &element = m_elements[name];
        if (!json.contains(name)) {
            element.type = ThemeElement::Null;
            continue;
        }
        auto value = json[name];
        if (value.isNull()) {
            element.type = ThemeElement::Null;
        } else if (value.isString()) {
            element.type = ThemeElement::Option;
            element.option = value.toString();
        } else { // Object
            element.type = ThemeElement::Color;
            element.color = to_color(json[names[i]].toObject());
        }
    }
    //merge(defaultThemeElements());
}

void ThemeState::merge(const Elements &elements) {
    for (auto i = 0; i < std::size(names); ++i) {
        auto &element = m_elements[names[i]];
        if (element.type == ThemeElement::Null) {
            element = elements[names[i]];
        }
    }
}

} // namespace xi
