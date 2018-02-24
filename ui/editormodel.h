#ifndef EDITORMODEL_H
#define EDITORMODEL_H

#include <QObject>

class EditorModel : public QObject
{
    Q_OBJECT
public:
    explicit EditorModel(QObject *parent = nullptr);

signals:

public slots:
};

#endif // EDITORMODEL_H