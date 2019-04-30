#ifndef CONTENT_VIEW_H
#define CONTENT_VIEW_H

#include <QAbstractScrollArea>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QGridLayout>
#include <QMargins>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPoint>
#include <QQueue>
#include <QScrollArea>
#include <QTimer>
#include <QLabel>

#include <memory>

#include "core_connection.h"
#include "file.h"
#include "font.h"
#include "line_cache.h"

namespace xi {

class Config;

class DataSource {
public:
    DataSource();
    std::shared_ptr<Font> defaultFont;
    std::shared_ptr<LineCache> lines;
    std::shared_ptr<Config> config;
    std::shared_ptr<QFontMetricsF> fontMetrics;
    qreal gutterWidth;
};

class LineColumn {
public:
    LineColumn(int line, int column) {
        m_lc.first = line;
        m_lc.second = column;
    }
    bool isValid() {
        return line() >= 0 && column() >= 0;
    }
    inline int line() const {
        return m_lc.first;
    }
    inline void line(int line) {
        m_lc.first = line;
    }
    inline int column() const {
        return m_lc.second;
    }
    inline void column(int column) {
        m_lc.second = column;
    }

private:
    QPair<int, int> m_lc;
};

class ContentView;

class AsyncPaintTimer : public QObject {
    Q_OBJECT
public:
    AsyncPaintTimer(QWidget *parent);

public slots:
    void update();

private:
    ContentView *m_contentView = nullptr;
    std::unique_ptr<QTimer> m_timer;
};

#define SEND_EDIT_METHOD(TypeName) \
    void TypeName() { sendEdit(m_selectorToCommand[#TypeName]); }

// Main Content
class ContentView : public QWidget {
    Q_OBJECT
public:
    friend class AsyncPaintTimer;

public:
    ContentView(std::shared_ptr<File> file, std::shared_ptr<CoreConnection> connection, QWidget *parent);

protected:
    virtual bool event(QEvent *e) override;
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) override;

	virtual QVariant inputMethodQuery(Qt::InputMethodQuery) const Q_DECL_OVERRIDE;
    virtual void inputMethodEvent(QInputMethodEvent *event) Q_DECL_OVERRIDE;
    void showImeComposition(const QString &text);

    void paint(QPainter &renderer, const QRect &dirtyRect);
    void initSelectCommand();
    void tick();

public:
    std::shared_ptr<File> getFile() const;
    int getLines();
    qreal getAscent();
    qreal getMaxLineWidth();
    QMarginsF getPadding();
    int getLinesHeight();
    int getContentHeight();
    int getLinespace();
    qreal getMaxCharWidth();
    qreal getAverageCharWidth();
    QPoint getScrollOrigin();
    int getXOff();

    int getLine(int y);
    int getColumn(int line, int x);

    int checkLineVisible(int line);

    qreal getWidth(int line, int column);
    qreal getAverageWidth(int line, int column);
    LineColumn getLineColumn(const QPoint &pos);
    ClosedRangeI getVisibleLinesRange(const QRect &bound);

    void asyncPaint(int ms = 100);

    void scrollY(int y);
    void scrollX(int x);

    void sendEdit(const QString &method);

    SEND_EDIT_METHOD(deleteBackward);
    SEND_EDIT_METHOD(deleteForward);
    SEND_EDIT_METHOD(insertNewline);
    SEND_EDIT_METHOD(insertTab);

    SEND_EDIT_METHOD(moveLeft);
    SEND_EDIT_METHOD(moveWordLeft);
    SEND_EDIT_METHOD(moveWordLeftAndModifySelection);
    SEND_EDIT_METHOD(moveLeftAndModifySelection);

    SEND_EDIT_METHOD(moveRight);
    SEND_EDIT_METHOD(moveWordRight);
    SEND_EDIT_METHOD(moveWordRightAndModifySelection);
    SEND_EDIT_METHOD(moveRightAndModifySelection);

    SEND_EDIT_METHOD(moveUp);
    SEND_EDIT_METHOD(moveUpAndModifySelection);

    SEND_EDIT_METHOD(moveDown);
    SEND_EDIT_METHOD(moveDownAndModifySelection);

    SEND_EDIT_METHOD(moveToBeginningOfLine);
    SEND_EDIT_METHOD(moveToBeginningOfDocumentAndModifySelection);
    SEND_EDIT_METHOD(moveToBeginningOfLineAndModifySelection);
    SEND_EDIT_METHOD(moveToBeginningOfDocument);

    SEND_EDIT_METHOD(moveToEndOfLine);
    SEND_EDIT_METHOD(moveToEndOfDocumentAndModifySelection);
    SEND_EDIT_METHOD(moveToEndOfLineAndModifySelection);
    SEND_EDIT_METHOD(moveToEndOfDocument);

    SEND_EDIT_METHOD(scrollPageDown);
    SEND_EDIT_METHOD(pageDownAndModifySelection);

    SEND_EDIT_METHOD(scrollPageUp);
    SEND_EDIT_METHOD(pageUpAndModifySelection);

    SEND_EDIT_METHOD(selectAll);

    SEND_EDIT_METHOD(uppercase);
    SEND_EDIT_METHOD(lowercase);
    SEND_EDIT_METHOD(undo);
    SEND_EDIT_METHOD(redo);

    void insertChar(const QString &text);
    void copy();
    void cut();
    void paste();

signals:
    void repaintContentReceived();

public slots:
    void repaintContentHandler();

public:
    void updateHandler(const QJsonObject &update);
    void scrollHandler(int line, int column);
    void pluginStartedHandler(const QString &pluginName);
    void pluginStoppedHandler(const QString &pluginName);
    void availablePluginsHandler(const QList<QJsonObject> &plugins);
    void updateCommandsHandler(const QStringList &commands);
    void configChangedHandler(const QJsonObject &changes);

    //II
    void themeChangedHandler();

private:
    std::unique_ptr<QLabel> m_imeComposition;
    QVector<QPoint> m_cursorCache;
    std::shared_ptr<File> m_file;
    std::shared_ptr<CoreConnection> m_connection;
    std::shared_ptr<DataSource> m_dataSource; //owned
    QPoint m_scrollOrigin;
    int m_firstLine;
    int m_visibleLines;
    qreal m_maxLineWidth;
    QHash<QString, QString> m_selectorToCommand;
    QMarginsF m_padding;
    bool m_drag = false;
    QTimer m_mouseDoubleCheckTimer;
    std::unique_ptr<AsyncPaintTimer> m_asyncPaintTimer;
    QQueue<qint64> m_asyncPaintQueue;
};

// focus performance
class ContentViewOpenGL : public QOpenGLWidget, protected QOpenGLFunctions {
public:
private:
};

} // namespace xi

#endif // CONTENT_VIEW_H