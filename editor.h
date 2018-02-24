#ifndef EDITOR_H
#define EDITOR_H

#include <QWidget>
#include <QVector>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPaintEvent>
#include <QRect>
#include <QMap>
#include <QHash>
#include <QChar>
#include <QDebug>
#include <QLabel>

#include "xibridge.h"

namespace xi {

class Line
{
    friend class LineCache;
public:
    Line();
    Line(const QJsonObject& value);
    void fromJson(const QJsonObject& value);
    QString text();
    QVector<int> cursors();

private:
    QString m_text;
    QVector<int> m_cursors;
};

class LineCache
{
    friend class Editor;
public:
    enum OpType {
        OP_COPY = 0,
        OP_SKIP,
        OP_INVALIDATE,
        OP_UPDATE,
        OP_INS,
        OP_UNKOWN,
    };

    int height();
    const Line& getLine(int idx);
    void applyUpdate(const QJsonObject& value);
    OpType opType(QString name);

private:
    static const QMap<OpType, QString> m_opmap;
    QVector<Line> m_lines;
};

struct EditorOption
{
    QColor bg;
    QColor fg;
    QFont font;
    int lineSpace;
    int baseLine;
};

class Editor : public QWidget
{
    Q_OBJECT
public:
    Editor(XiBridge* bridge, const EditorOption& option, QWidget *parent = nullptr);
    void new_view(QString file = "");
    void setOption(const EditorOption& option);
    static EditorOption getDefaultOption();

    QString getViewId();
    void setViewId(QString id);

    virtual QVariant inputMethodQuery(Qt::InputMethodQuery) const Q_DECL_OVERRIDE;
    QPoint cursorPos(int ix = 0) const;

signals:
    void editorOptionChanged();

public slots:
    void updateEditorCache(const QJsonObject& data);

protected:
    virtual void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    virtual void inputMethodEvent(QInputMethodEvent *event) Q_DECL_OVERRIDE;
    void showImeComposition(const QString& text);

private:
    QString m_viewId;

    QVector<QPoint> m_cursors;
    QLabel *m_imeComposition;
    EditorOption m_option;
    XiBridge *m_bridge;
    LineCache m_cache;
};
}



#endif // EDITOR_H
