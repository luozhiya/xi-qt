#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QObject>

class Configuration : public QObject
{
    Q_OBJECT
public:
    explicit Configuration(QObject *parent = nullptr);

signals:

public slots:

private:
//    FontConfig m_cjk;
//    FontConfig m_default;
//    bool m_cjkEnable;
};

#endif // CONFIGURATION_H
