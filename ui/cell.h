#ifndef CELL_H
#define CELL_H

#include <QObject>

class Cell : public QObject
{
    Q_OBJECT
public:
    explicit Cell(QObject *parent = nullptr);

signals:

public slots:

private:
//    Font* m_font; //
};

#endif // CELL_H
