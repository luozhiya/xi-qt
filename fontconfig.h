#ifndef FONTCONFIG_H
#define FONTCONFIG_H

#include <QObject>

class FontConfig : public QObject
{
    Q_OBJECT
public:
    explicit FontConfig(QObject *parent = nullptr);

signals:

public slots:

private:
    QString m_name;
    int     m_size;
    int     m_baseline;
};

#endif // FONTCONFIG_H
