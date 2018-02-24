#ifndef FONTCACHE_H
#define FONTCACHE_H

#include <QObject>

class FontCache : public QObject
{
    Q_OBJECT
public:
    explicit FontCache(QObject *parent = nullptr);

signals:

public slots:
};

#endif // FONTCACHE_H