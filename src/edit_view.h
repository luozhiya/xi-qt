#ifndef EDIT_VIEW_H
#define EDIT_VIEW_H

#include <QWidget>

#include <memory>

#include "core_connection.h"
#include "file.h"
#include "font.h"

class QScrollBar;

namespace xi {

class EditWindow;
class ContentView;
class ScrollTester;
class FpsCounterWidget;

// Containner
class EditView : public QWidget {
    Q_OBJECT
public:
    friend class ScrollTester;

    explicit EditView(std::shared_ptr<File> file, std::shared_ptr<CoreConnection> connection, QWidget *parent = nullptr);
    ~EditView();

    std::shared_ptr<File> getFile() const;
    void relayoutScrollBar();
    void focusOnEdit();
    void tick();

public:
    void updateHandler(const QJsonObject &json);
    void scrollHandler(int line, int column);
    void themeChangedHandler();
    void configChangedHandler(const QJsonObject &changes);

protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void wheelEvent(QWheelEvent *event) override;

public slots:
    void scrollBarVChanged(int y);
    void scrollBarHChanged(int x);
    void scrollTester();

private:
    EditWindow *m_editWindow;
    ContentView *m_content;
    QScrollBar *m_scrollBarV;
    QScrollBar *m_scrollBarH;
    FpsCounterWidget *m_fpsCounter;
    std::unique_ptr<ScrollTester> m_scrollTester;
};

class GutterWidget : public QWidget {
public:
private:
};

class ScrollTester : public QObject {
    Q_OBJECT
public:
    enum State {
        NotRunning,
        Running,
    };
    enum Direction {
        Up,
        Down,
    };

    ScrollTester(EditView *view);
    ~ScrollTester();
    void mode();
    void update();

private:
    State m_state = NotRunning;
    Direction m_direction = Up;
    std::unique_ptr<QTimer> m_timer;
    EditView *m_view = nullptr;
};

class FpsCounterWidget : public QWidget {
public:
    FpsCounterWidget(QWidget *parent = nullptr);

    void update();

    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

    void tick();

    int getContentHeight();
    int getAverageCharWidth();

private:
    qint64 m_current = 0;
    qint64 m_tick = 0;
    qint64 m_tickCache = 0;
    int m_fpsCache = 0;
    bool m_update = false;
    std::unique_ptr<QTimer> m_timer;
    EditView *m_view = nullptr;
    std::unique_ptr<QFontMetrics> m_metrics;
};

} // namespace xi

#endif // EDIT_VIEW_H
