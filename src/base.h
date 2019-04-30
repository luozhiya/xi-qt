#ifndef BASE_H
#define BASE_H

#include <QString>
#include <magic_enum.hpp>

namespace xi {

template <typename E>
E to_enum(const QString &name, E default) {
    static_assert(std::is_enum_v<E>, "requires enum type.");

    auto result = magic_enum::enum_cast<E>(name.toStdString());
    if (result.has_value()) {
        return result.value();
    }
    return default;
}

template <typename E>
QString to_string(E e) {
    static_assert(std::is_enum_v<E>, "requires enum type.");

    auto name = magic_enum::enum_name(e);
    if (name.has_value()) {
        return name.value().data();
    }
    return QString();
}

} // namespace xi

#endif // BASE_H