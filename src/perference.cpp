#include "perference.h"

namespace xi {

Perference::Perference() {
    m_styleMap = std::make_shared<StyleMap>();
    m_theme = std::make_shared<Theme>();
    m_config = std::make_shared<Config>();
}

Perference *Perference::shared() {
    static Perference perference;
    return &perference;
}

} // namespace xi