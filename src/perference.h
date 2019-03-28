#ifndef PERFERENCE_H
#define PERFERENCE_H

#include <memory>

#include "style_map.h"
#include "theme.h"
#include "config.h"

namespace xi {

class Perference {
public:
    static Perference *shared();

    std::shared_ptr<Theme> theme() const {
        return m_theme;
    }

    std::shared_ptr<StyleMap> styleMap() const {
        return m_styleMap;
    }

    std::shared_ptr<Config> config() const {
        return m_config;
    }

private:
    Perference();

    std::shared_ptr<Theme> m_theme;
    std::shared_ptr<StyleMap> m_styleMap;
    std::shared_ptr<Config> m_config;
};

} // namespace xi

#endif // PERFERENCE_H