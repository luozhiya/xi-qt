#include "file.h"

#include <QFileInfo>

namespace xi {

File::File() {
}

File::File(const File &file) {
    m_path = file.m_path;
    m_viewId = file.m_viewId;
}

void File::setPath(const QString &path) {
    m_path = path;
}

QString File::path() const {
    return m_path;
}

void File::setViewId(const QString &viewId) {
    m_viewId = viewId;
}

void File::setTempName(const QString &name) {
    m_tempName = name;
}

QString File::viewId() const {
    return m_viewId;
}

QString File::name() const {
    QFileInfo info = m_path;
    return info.fileName();
}

QString File::tempName() const {
    return m_tempName;
}

QString File::displayName() const {
    return m_path.isEmpty() ? tempName() : name();
}

} // namespace xi