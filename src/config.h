#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QStringList>
#include <QVariant>

#include <memory>

#include "unfair_lock.h"

namespace xi {

#define CONFIG_STATE_METHOD(TypeName)                                           \
    void TypeName(const QVariant &variant) { m_elements[#TypeName] = variant; } \
    QVariant TypeName() { return m_elements[#TypeName]; }

#define CONFIG_LOCKED_METHOD(TypeName)                                     \
    void TypeName(const QVariant &variant) { m_inner->TypeName(variant); } \
    QVariant TypeName() { return m_inner->TypeName(); }

class ConfigState : public UnfairLock {
public:
    using Elements = QHash<QString, QVariant>;

    ConfigState() {
    }

    void applyUpdate(const QJsonObject &json);

    CONFIG_STATE_METHOD(auto_indent);
    CONFIG_STATE_METHOD(font_face);
    CONFIG_STATE_METHOD(font_size);
    CONFIG_STATE_METHOD(line_ending);
    CONFIG_STATE_METHOD(plugin_search_path);
    CONFIG_STATE_METHOD(scroll_past_end);
    CONFIG_STATE_METHOD(tab_size);
    CONFIG_STATE_METHOD(translate_tabs_to_spaces);
    CONFIG_STATE_METHOD(use_tab_stops);
    CONFIG_STATE_METHOD(word_wrap);
    CONFIG_STATE_METHOD(wrap_width);

private:
    Elements m_elements;
};

class ConfigLocked {
public:
    ConfigLocked(std::shared_ptr<ConfigState> mutex) {
        m_inner = mutex;
        m_inner->lock();
    }
    ~ConfigLocked() {
        m_inner->unlock();
    }

    void applyUpdate(const QJsonObject &json){
        m_inner->applyUpdate(json);
    }

    CONFIG_LOCKED_METHOD(auto_indent);
    CONFIG_LOCKED_METHOD(font_face);
    CONFIG_LOCKED_METHOD(font_size);
    CONFIG_LOCKED_METHOD(line_ending);
    CONFIG_LOCKED_METHOD(plugin_search_path);
    CONFIG_LOCKED_METHOD(scroll_past_end);
    CONFIG_LOCKED_METHOD(tab_size);
    CONFIG_LOCKED_METHOD(translate_tabs_to_spaces);
    CONFIG_LOCKED_METHOD(use_tab_stops);
    CONFIG_LOCKED_METHOD(word_wrap);
    CONFIG_LOCKED_METHOD(wrap_width);

private:
    std::shared_ptr<ConfigState> m_inner;
};

class Config {
public:
    Config() {
        m_state = std::make_shared<ConfigState>();
    }

    std::shared_ptr<ConfigLocked> locked() {
        return std::make_shared<ConfigLocked>(m_state);
    }

private:
    std::shared_ptr<ConfigState> m_state;
};

} // namespace xi

#endif // CONFIG_H
