#ifndef XIFILE_H
#define XIFILE_H

#include <QString>
#include <QVector>

namespace xi {

class File {
public:
    File();
    File(const File &file);

    void setPath(const QString &path);
    void setViewId(const QString &viewId);
    void setTempName(const QString &name);

    QString viewId() const;
    QString path() const;
    QString name() const;
    QString tempName() const;
    QString displayName() const;

private:
    QString m_path;
    QString m_viewId;
    QString m_tempName; //new file save dialog
};

} // namespace xi

#endif // FILE_H