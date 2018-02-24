#ifndef LINE_H
#define LINE_H

#include <QObject>

class Line : public QObject
{
    Q_OBJECT
public:
    explicit Line(QObject *parent = nullptr);

signals:

public slots:
};

#endif // LINE_H