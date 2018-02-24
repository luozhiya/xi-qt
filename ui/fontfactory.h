#ifndef FONTFACTORY_H
#define FONTFACTORY_H

#include <QObject>

class FontFactory : public QObject
{
    Q_OBJECT
public:
    explicit FontFactory(QObject *parent = nullptr);

signals:

public slots:
};

#endif // FONTFACTORY_H