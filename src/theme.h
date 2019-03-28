#ifndef THEME_H
#define THEME_H

#include <QBrush>
#include <QColor>
#include <QHash>
#include <QJsonObject>
#include <QString>

#include "unfair_lock.h"

namespace xi {

struct ThemeElement {
    enum Type {
        Color,
        Option,
        Null,
    };

    QColor color;
    QString option;
    Type type;

    ThemeElement() {
        type = Null;
    }
    ~ThemeElement() {
    }
    ThemeElement(const QColor &color) {
        type = Color;
        this->color = color;
    }
    ThemeElement(Qt::GlobalColor gcolor) {
        ThemeElement(QColor(gcolor));
    }
    ThemeElement(const ThemeElement &ele) {
        *this = ele;
    }
    ThemeElement &operator=(const ThemeElement &ele) {
        type = ele.type;
        if (type == Color) {
            color = ele.color;
        } else if (type == Option) {
            option = ele.option;
        }
        return *this;
    }
    operator QColor() {
        return color;
    }
    operator QString() {
        return option;
    }
};

#define THEME_ELEMENT_METHOD(TypeName)                                                     \
    inline void TypeName(const ThemeElement &element) { m_elements[#TypeName] = element; } \
    inline ThemeElement TypeName() const { return m_elements[#TypeName]; }

#define THEME_LOCKED_METHOD(TypeName)                                                 \
    inline void TypeName(const ThemeElement &element) { m_inner->TypeName(element); } \
    inline ThemeElement TypeName() const { return m_inner->TypeName(); }

class ThemeState : public UnfairLock {
public:
    using Elements = QHash<QString, ThemeElement>;

    ThemeState();

    void applyUpdate(const QString &name, const QJsonObject &json);

    THEME_ELEMENT_METHOD(accent);
    THEME_ELEMENT_METHOD(active_guide);
    THEME_ELEMENT_METHOD(background);
    THEME_ELEMENT_METHOD(bracket_contents_foreground);
    THEME_ELEMENT_METHOD(bracket_contents_options);
    THEME_ELEMENT_METHOD(brackets_background);
    THEME_ELEMENT_METHOD(brackets_foreground);
    THEME_ELEMENT_METHOD(brackets_options);
    THEME_ELEMENT_METHOD(caret);
    THEME_ELEMENT_METHOD(find_highlight);
    THEME_ELEMENT_METHOD(find_highlight_foreground);
    THEME_ELEMENT_METHOD(foreground);
    THEME_ELEMENT_METHOD(guide);
    THEME_ELEMENT_METHOD(gutter);
    THEME_ELEMENT_METHOD(gutter_foreground);
    THEME_ELEMENT_METHOD(highlight);
    THEME_ELEMENT_METHOD(highlight_foreground);
    THEME_ELEMENT_METHOD(inactive_selection);
    THEME_ELEMENT_METHOD(inactive_selection_foreground);
    THEME_ELEMENT_METHOD(line_highlight);
    THEME_ELEMENT_METHOD(minimap_border);
    THEME_ELEMENT_METHOD(misspelling);
    THEME_ELEMENT_METHOD(phantom_css);
    THEME_ELEMENT_METHOD(popup_css);
    THEME_ELEMENT_METHOD(selection);
    THEME_ELEMENT_METHOD(selection_background);
    THEME_ELEMENT_METHOD(selection_border);
    THEME_ELEMENT_METHOD(selection_foreground);
    THEME_ELEMENT_METHOD(shadow);
    THEME_ELEMENT_METHOD(stack_guide);
    THEME_ELEMENT_METHOD(tags_foreground);
    THEME_ELEMENT_METHOD(tags_options);

private:
    void merge(const Elements &elements);
    QString m_name;
    Elements m_elements;
};

class ThemeLocked {
public:
    ThemeLocked(const std::shared_ptr<ThemeState> &mutex) {
        m_inner = mutex;
        m_inner->lock();
    }
    ~ThemeLocked() {
        m_inner->unlock();
    }

    void applyUpdate(const QString &name, const QJsonObject &json) {
        m_inner->applyUpdate(name, json);
    }

    THEME_LOCKED_METHOD(accent);
    THEME_LOCKED_METHOD(active_guide);
    THEME_LOCKED_METHOD(background);
    THEME_LOCKED_METHOD(bracket_contents_foreground);
    THEME_LOCKED_METHOD(bracket_contents_options);
    THEME_LOCKED_METHOD(brackets_background);
    THEME_LOCKED_METHOD(brackets_foreground);
    THEME_LOCKED_METHOD(brackets_options);
    THEME_LOCKED_METHOD(caret);
    THEME_LOCKED_METHOD(find_highlight);
    THEME_LOCKED_METHOD(find_highlight_foreground);
    THEME_LOCKED_METHOD(foreground);
    THEME_LOCKED_METHOD(guide);
    THEME_LOCKED_METHOD(gutter);
    THEME_LOCKED_METHOD(gutter_foreground);
    THEME_LOCKED_METHOD(highlight);
    THEME_LOCKED_METHOD(highlight_foreground);
    THEME_LOCKED_METHOD(inactive_selection);
    THEME_LOCKED_METHOD(inactive_selection_foreground);
    THEME_LOCKED_METHOD(line_highlight);
    THEME_LOCKED_METHOD(minimap_border);
    THEME_LOCKED_METHOD(misspelling);
    THEME_LOCKED_METHOD(phantom_css);
    THEME_LOCKED_METHOD(popup_css);
    THEME_LOCKED_METHOD(selection);
    THEME_LOCKED_METHOD(selection_background);
    THEME_LOCKED_METHOD(selection_border);
    THEME_LOCKED_METHOD(selection_foreground);
    THEME_LOCKED_METHOD(shadow);
    THEME_LOCKED_METHOD(stack_guide);
    THEME_LOCKED_METHOD(tags_foreground);
    THEME_LOCKED_METHOD(tags_options);

private:
    std::shared_ptr<ThemeState> m_inner;
};

class Theme {
public:
    Theme() {
        m_state = std::make_shared<ThemeState>();
    }
    explicit Theme(std::shared_ptr<ThemeState> state) {
        m_state = state;
    }

    inline std::shared_ptr<ThemeLocked> locked() {
        return std::make_shared<ThemeLocked>(m_state);
    }

private:
    std::shared_ptr<ThemeState> m_state;
};

using ThemeList = QList<Theme>;

} // namespace xi

#endif // THEME_H
