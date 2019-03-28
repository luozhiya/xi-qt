#include "style.h"

namespace xi {

int utf8OffsetToUtf16(const QString &text, int ix) {
    QString utf16 = text.toUtf8().left(ix);
    return utf16.length();
}

} // namespace xi
