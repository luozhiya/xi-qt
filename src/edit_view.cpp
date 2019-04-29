#include "edit_view.h"

#include <QAbstractScrollArea>
#include <QApplication>
#include <QClipboard>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMimeData>
#include <QPainter>
#include <QPoint>
#include <QScrollArea>
#include <QScrollBar>

#include <cmath>

#include "content_view.h"
#include "edit_window.h"
#include "perference.h"
#include "shortcuts.h"
#include "text_line.h"

namespace xi {

static constexpr const char *FPS_FONT = "Inconsolata";

EditView::EditView(std::shared_ptr<File> file, std::shared_ptr<CoreConnection> connection, QWidget *parent) : QWidget(parent) {

    setFocusPolicy(Qt::FocusPolicy::NoFocus);
    setAttribute(Qt::WA_KeyCompression, false);
    setAttribute(Qt::WA_StaticContents);
    setAttribute(Qt::WA_Resized);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContentsMargins(0, 0, 0, 0);

    m_content = new ContentView(file, connection, this);
    m_scrollBarV = new QScrollBar(this);
    m_scrollBarH = new QScrollBar(Qt::Horizontal, this);
    QGridLayout *layout = new QGridLayout();
    layout->addWidget(m_content, 1, 1);
    layout->addWidget(m_scrollBarV, 1, 2);
    layout->addWidget(m_scrollBarH, 2, 1);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setMargin(0);
    layout->setSpacing(0);
    this->setLayout(layout);

    connect(m_scrollBarV, &QScrollBar::valueChanged, this, &EditView::scrollBarVChanged);
    connect(m_scrollBarH, &QScrollBar::valueChanged, this, &EditView::scrollBarHChanged);

    //QFile qss(":/resources/qss/scroll_bar.qss");
    //qss.open(QFile::ReadOnly);
    //auto scrollBarStyle = qss.readAll();
    //m_scrollBarV->setStyleSheet(scrollBarStyle);
    //m_scrollBarH->setStyleSheet(scrollBarStyle);

    m_scrollTester = std::make_unique<ScrollTester>(this);

    auto seq = QKeySequence("F8");
    Shortcuts::shared()->append(this, seq, [&](QShortcut *shortcut) {
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(shortcut, &QShortcut::activated,
                this, &EditView::scrollTester);
    });

    m_fpsCounter = new FpsCounterWidget(this);
}

EditView::~EditView() {
    m_scrollTester.reset();
}

void EditView::updateHandler(const QJsonObject &json) {
    m_content->updateHandler(json);
    relayoutScrollBar();
}

void EditView::scrollHandler(int line, int column) {
    auto linespace = m_content->getLinespace();
    auto vValue = m_scrollBarV->value();
    auto nextLine = line + 1;

    relayoutScrollBar();

    if (nextLine * linespace > vValue + m_content->height()) {
        m_scrollBarV->setValue(nextLine * linespace - m_content->height());
    }
    if (line * linespace < vValue) {
        m_scrollBarV->setValue(line * linespace);
    }

    auto scrollOriginX = m_content->getScrollOrigin().x();
    auto lcwidth = m_content->getWidth(line, column);
    auto contentWidth = m_content->width();
    auto averageCharWidth = m_content->getAverageCharWidth();

    if (lcwidth == -1) { // no cache
        // simple
        lcwidth = m_content->getAverageWidth(line, column);
    }
    auto delta = lcwidth - scrollOriginX;
    if (delta <= 0 || delta >= contentWidth - averageCharWidth) {
        m_scrollBarH->setValue(qMax(0, int(lcwidth - contentWidth / 2)));
    }

    m_content->scrollHandler(line, column);
}

void EditView::themeChangedHandler() {
    //Scrollbar
    //Content
    m_content->themeChangedHandler();
}

void EditView::configChangedHandler(const QJsonObject &changes) {
    m_content->configChangedHandler(changes);
}

void EditView::resizeEvent(QResizeEvent *event) {
    relayoutScrollBar();

    auto viewrc = rect();
    auto textWidth = m_fpsCounter->getAverageCharWidth() * 3;
    QRect fpsrc(viewrc.width() - textWidth - m_scrollBarV->width(), 0, textWidth, m_fpsCounter->getContentHeight());
    m_fpsCounter->setGeometry(fpsrc);

    QWidget::resizeEvent(event);
}

void EditView::keyPressEvent(QKeyEvent *e) {
    QWidget::keyPressEvent(e);
}

void EditView::relayoutScrollBar() {
    auto widgetHeight = m_content->height();
    auto widgetWidth = m_content->width();
    auto linespace = m_content->getLinespace();

    auto contentHeight = m_content->getContentHeight();
    m_scrollBarV->setRange(0, contentHeight - linespace); // keep the last line
    m_scrollBarV->setSingleStep(m_content->getLinespace());
    m_scrollBarV->setPageStep(widgetHeight);
    m_scrollBarV->setVisible(contentHeight > widgetHeight);

    auto maxLineWidth = m_content->getMaxLineWidth();
    auto maxCharWidth = m_content->getMaxCharWidth();
    m_scrollBarH->setRange(0, maxLineWidth - maxCharWidth);
    m_scrollBarH->setSingleStep(maxCharWidth);
    m_scrollBarH->setPageStep(widgetWidth);
    //auto xOff = m_content->getXOff();
    //auto visible = (maxLineWidth + xOff + maxCharWidth / 2.f > widgetWidth);
    //if (!visible && m_content->getScrollOrigin().x() != 0) visible = true;
    //m_scrollBarH->setVisible(visible);
    m_scrollBarH->setVisible(true);
}

void EditView::wheelEvent(QWheelEvent *event) {
    if (m_scrollBarV->isVisible()) {
        auto linespace = m_content->getLinespace();
        auto delta = -(event->delta() / (120) * (linespace * 3));
        auto v = m_scrollBarV->value();
        m_scrollBarV->setValue(v + delta);
    }
}

std::shared_ptr<File> EditView::getFile() const {
    return m_content->getFile();
}

void EditView::focusOnEdit() {
    m_content->setFocus();
    //m_content->update();
}

void EditView::tick() {
    m_fpsCounter->tick();
}

void EditView::scrollBarVChanged(int y) {
    m_content->scrollY(y);
    relayoutScrollBar();
}

void EditView::scrollBarHChanged(int x) {
    m_content->scrollX(x);
    relayoutScrollBar();
}

void EditView::scrollTester() {
    m_scrollTester->mode();
}

ScrollTester::ScrollTester(EditView *view) : m_view(view) {
    m_timer = std::make_unique<QTimer>(this);
    connect(m_timer.get(), &QTimer::timeout, this, &ScrollTester::update);
}

ScrollTester::~ScrollTester() {
    m_timer->stop();
    m_state = NotRunning;
}

void ScrollTester::mode() {
    if (m_state == NotRunning) {
        m_timer->start(5);
        m_state = Running;
    } else if (m_state == Running) {
        m_timer->stop();
        m_state = NotRunning;
    }
}

void ScrollTester::update() {
    if (m_view) {
        auto value = m_view->m_scrollBarV->value();
        if (value == m_view->m_scrollBarV->maximum()) {
            m_direction = Up;
        } else if (value == m_view->m_scrollBarV->minimum()) {
            m_direction = Down;
        }
        auto linespace = m_view->m_content->getLinespace();
        auto lines = linespace * 10;
        auto delta = m_direction == Down ? lines : -lines;
        m_view->m_scrollBarV->setValue(value + delta);
    }
}

FpsCounterWidget::FpsCounterWidget(QWidget *parent /*= nullptr*/) : QWidget(parent) {
    //setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_InputMethodTransparent);

    m_view = dynamic_cast<EditView *>(parent);
    m_timer = std::make_unique<QTimer>(this);
    connect(m_timer.get(), &QTimer::timeout, this, &FpsCounterWidget::update);
    m_timer->start(100);

    QString family = FPS_FONT;
    int size = 11;
    int weight = QFont::Normal;
    bool italic = false;
    QFont font(family, size, weight, italic);
    font.setStyleHint(QFont::Monospace, QFont::StyleStrategy(QFont::PreferDefault | QFont::ForceIntegerMetrics));
    font.setFixedPitch(true);
    font.setKerning(false);
    setFont(font);
    m_metrics = std::make_unique<QFontMetrics>(font);

    setContentsMargins(0, 0, 0, 0);
}

void FpsCounterWidget::update() {
    m_tickCache = m_tick;
    m_tick = 0;
    m_update = true;
    repaint();
}

void FpsCounterWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    if (m_update) {
        using namespace std::chrono;
        auto timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
        auto timeDuration = (timestamp - m_current); // /1000000.f
        int fps = std::floor(qreal(m_tickCache) * 1000000 / timeDuration + 0.499);
        m_current = timestamp;
        m_update = false;
        if (fps != 0)
            m_fpsCache = fps;
    }
    auto bgrc = rect();
    QString fpsString = QString::number(m_fpsCache);

    // 1, 80, 198
    // 128, 128, 128
    painter.fillRect(bgrc, QColor::fromRgb(1, 80, 198, 255));
    painter.setPen(QColor::fromRgb(255, 255, 255, 255));
    painter.drawText(QPoint(bgrc.left(), m_metrics->ascent()), fpsString);
}

void FpsCounterWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

void FpsCounterWidget::tick() {
    ++m_tick;
}

int FpsCounterWidget::getContentHeight() {
    return m_metrics->height();
}

int FpsCounterWidget::getAverageCharWidth() {
    return m_metrics->averageCharWidth();
}

} // namespace xi
