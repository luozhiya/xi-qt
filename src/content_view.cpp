#include "content_view.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QThreadPool>
#include <QtConcurrent>

#include "config.h"
#include "edit_view.h"
#include "perference.h"
#include "theme.h"

namespace xi {

static constexpr const char *CONTENT_FONT = "Inconsolata";

ContentView::ContentView(
    std::shared_ptr<File> file,
    std::shared_ptr<CoreConnection> connection,
    QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_KeyCompression, false);
    setAttribute(Qt::WA_KeyboardFocusChange);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setAutoFillBackground(false);

    m_connection = connection;
    m_file = file;
    m_firstLine = 0;
    m_visibleLines = 0;
    m_maxLineWidth = 0;

    m_dataSource = std::make_shared<DataSource>();

    m_imeComposition = std::make_unique<QLabel>(this);
    m_imeComposition->setVisible(false);
    m_imeComposition->setTextFormat(Qt::PlainText);
    m_imeComposition->setTextInteractionFlags(Qt::NoTextInteraction);
    m_imeComposition->setAutoFillBackground(true);
    m_imeComposition->setFont(m_dataSource->defaultFont->getFont());

    m_padding.setLeft(2);
    m_padding.setTop(0);
    m_padding.setRight(0);
    m_padding.setBottom(0);

    m_asyncPaintTimer = std::make_unique<AsyncPaintTimer>(this);

    connect(this, &ContentView::repaintContentReceived, this, &ContentView::repaintContentHandler);

    initSelectCommand();
}

bool ContentView::event(QEvent *e) {
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Tab) {
            insertTab();
            return true;
        }
    }
    return QWidget::event(e);
}

void ContentView::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    auto dirtyRect = event->rect();
    paint(painter, dirtyRect);
    tick();
}

void ContentView::resizeEvent(QResizeEvent *event) {
    auto size = event->size();

    auto linespace = getLinespace();
    auto visibleLines = std::ceil(size.height() / qreal(linespace));
    if (m_visibleLines != visibleLines) {
        m_visibleLines = visibleLines;
        m_connection->sendScroll(m_file->viewId(), m_firstLine, m_firstLine + m_visibleLines);
    }
    QWidget::resizeEvent(event);
}

void ContentView::sendEdit(const QString &method) {
    if (!method.isEmpty()) {
        QJsonObject object;
        m_connection->sendEdit(m_file->viewId(), method, object);
    }
}

ClosedRangeI ContentView::getVisibleLinesRange(const QRect &bound) {
    auto linespace = getLinespace();
    auto firstVisible = qMax(0, (int)(std::ceil((bound.y() - m_padding.top() + m_scrollOrigin.y()) / linespace)));
    auto lastVisible = qMax(0, (int)(std::ceil((bound.y() + bound.height() - m_padding.top() + m_scrollOrigin.y()) / linespace)));
    return ClosedRangeI(firstVisible, lastVisible);
}

void ContentView::paint(QPainter &renderer, const QRect &dirtyRect) {
    auto lineCache = m_dataSource->lines->locked();
    auto linespace = m_dataSource->fontMetrics->height();
    m_padding.setTop(linespace - m_dataSource->fontMetrics->ascent());
    auto xOff = m_dataSource->gutterWidth + m_padding.left() - m_scrollOrigin.x();
    auto yOff = m_padding.top() - m_scrollOrigin.y();

    auto firstVisible = qMax(0, (int)(std::ceil((dirtyRect.y() - m_padding.top() + m_scrollOrigin.y()) / linespace)));
    auto lastVisible = qMax(0, (int)(std::ceil((dirtyRect.y() + dirtyRect.height() - m_padding.top() + m_scrollOrigin.y()) / linespace)));

    auto totalLines = lineCache->height();

    auto first = qMin(totalLines, firstVisible);
    auto last = qMin(totalLines, lastVisible);

    auto fetchRange = RangeI(first, last);
    auto lines = lineCache->blockingGet(fetchRange);

    if (lineCache->isMissingLines(lines, fetchRange)) {
        return;
    }

	m_dataSource->gutterWidth = m_dataSource->gutterOne * QString::number(totalLines).count() + 30;

    auto font = m_dataSource->defaultFont;
    auto theme = Perference::shared()->theme()->locked();
    auto styleMap = Perference::shared()->styleMap()->locked();

    QList<std::shared_ptr<TextLine>> textLines;

    m_firstLine = first;
    qreal maxLineWidth = 0;

    // background
    renderer.fillRect(dirtyRect, theme->background());

    // first pass: create TextLine objects and also draw background rects
    for (auto lineIx = first; lineIx < last; ++lineIx) {
        auto relLineIx = lineIx - first;
        auto &line = lines[relLineIx];
        if (!line) {
            textLines.append(nullptr);
            continue;
        }
        auto textLine = line->assoc();
        if (textLine) {
            textLines.append(textLine);
        } else {
            auto builder = std::make_shared<TextLineBuilder>(line->getText(), font);
            builder->setFgColor(theme->foreground());
            styleMap->applyStyles(builder, line->getStyles(), theme->selection(), theme->highlight());
            textLine = builder->build();
            textLines.append(textLine);
            line->setAssoc(textLine);
            //auto y0 = yOff + linespace * lineIx;
            //RangeF yRange(y0, y0 + linespace);
            //Painter::drawLineBg(renderer, textLine, xOff, yRange);
        }
        maxLineWidth = qMax(maxLineWidth, textLine->width());
    }

    m_maxLineWidth = maxLineWidth;

    // second pass: draw text & sel background
    for (auto lineIx = first; lineIx < last; ++lineIx) {
        auto textLine = textLines[lineIx - first];
        if (textLine) {
            auto y = yOff + m_dataSource->fontMetrics->ascent() - linespace + linespace * lineIx;
            Painter::drawLine(renderer, textLine, xOff, y);
        }
    }

    // third pass: draw text decorations
    //for (auto lineIx = first; lineIx < last; ++lineIx) {
    //	auto textLine = textLines[lineIx - first];
    //	if (textLine) {
    //		auto y = yOff + m_dataSource->fontMetrics->ascent() + linespace + lineIx;
    //		Renderer::drawLineDecorations(renderer, textLine, xOff, y);
    //	}
    //}

	m_cursorCache.clear();
    // fourth pass: draw carets
    for (auto lineIx = first; lineIx < last; ++lineIx) {
        auto relLineIx = lineIx - first;
        auto textLine = textLines[relLineIx];
        auto line = lines[relLineIx];
        if (textLine && line) {
            auto y0 = yOff + m_dataSource->fontMetrics->ascent() - linespace + linespace * lineIx;
            auto cursors = line->getCursor();
            foreach (int cursor, *cursors) {
                auto x0 = xOff + textLine->indexTox(cursor) - 0.5f;
                Painter::drawCursor(renderer, x0, y0, 2, linespace, theme->caret());
                m_cursorCache.push_back(QPoint(x0, y0));
			}
        }
    }

    // gutter drawing
    QRect gutterRect = {
        0,
        0,
        m_dataSource->gutterWidth,
        dirtyRect.height()};
    renderer.fillRect(gutterRect, theme->gutter());
    for (auto lineIx = first; lineIx < last; ++lineIx) {
        auto relLineIx = lineIx - first;
        auto line = lines[relLineIx];
        if (!line) continue;
        auto gutterNumber = line->number();
        auto x = 10;
        auto y0 = yOff + m_dataSource->fontMetrics->ascent() + linespace * (lineIx - 1);
        auto builder = std::make_shared<TextLineBuilder>(QString::number(gutterNumber), font);
        builder->setFgColor(QColor(255, 255, 255)); // theme->gutter_foreground()
        auto textLine = builder->build(true);
        Painter::drawLine(renderer, textLine, x, y0);
    }
}

void ContentView::initSelectCommand() {
    m_selectorToCommand["deleteBackward"] = "delete_backward";
    m_selectorToCommand["deleteForward"] = "delete_forward";
    m_selectorToCommand["deleteToBeginningOfLine"] = "delete_to_beginning_of_line";
    m_selectorToCommand["deleteToEndOfParagraph"] = "delete_to_end_of_paragraph";
    m_selectorToCommand["deleteWordBackward"] = "delete_word_backward";
    m_selectorToCommand["deleteWordForward"] = "delete_word_forward";
    m_selectorToCommand["insertNewline"] = "insert_newline";
    m_selectorToCommand["insertTab"] = "insert_tab";
    m_selectorToCommand["moveBackward"] = "move_backward";
    m_selectorToCommand["moveDown"] = "move_down";
    m_selectorToCommand["moveDownAndModifySelection"] = "move_down_and_modify_selection";
    m_selectorToCommand["moveForward"] = "move_forward";
    m_selectorToCommand["moveLeft"] = "move_left";
    m_selectorToCommand["moveLeftAndModifySelection"] = "move_left_and_modify_selection";
    m_selectorToCommand["moveRight"] = "move_right";
    m_selectorToCommand["moveRightAndModifySelection"] = "move_right_and_modify_selection";
    m_selectorToCommand["moveToBeginningOfDocument"] = "move_to_beginning_of_document";
    m_selectorToCommand["moveToBeginningOfDocumentAndModifySelection"] = "move_to_beginning_of_document_and_modify_selection";
    m_selectorToCommand["moveToBeginningOfLine"] = "move_to_left_end_of_line";
    m_selectorToCommand["moveToBeginningOfLineAndModifySelection"] = "move_to_left_end_of_line_and_modify_selection";
    m_selectorToCommand["moveToBeginningOfParagraph"] = "move_to_beginning_of_paragraph";
    m_selectorToCommand["moveToEndOfDocument"] = "move_to_end_of_document";
    m_selectorToCommand["moveToEndOfDocumentAndModifySelection"] = "move_to_end_of_document_and_modify_selection";
    m_selectorToCommand["moveToEndOfLine"] = "move_to_right_end_of_line";
    m_selectorToCommand["moveToEndOfLineAndModifySelection"] = "move_to_right_end_of_line_and_modify_selection";
    m_selectorToCommand["moveToEndOfParagraph"] = "move_to_end_of_paragraph";
    m_selectorToCommand["moveToLeftEndOfLine"] = "move_to_left_end_of_line";
    m_selectorToCommand["moveToLeftEndOfLineAndModifySelection"] = "move_to_left_end_of_line_and_modify_selection";
    m_selectorToCommand["moveToRightEndOfLine"] = "move_to_right_end_of_line";
    m_selectorToCommand["moveToRightEndOfLineAndModifySelection"] = "move_to_right_end_of_line_and_modify_selection";
    m_selectorToCommand["moveUp"] = "move_up";
    m_selectorToCommand["moveUpAndModifySelection"] = "move_up_and_modify_selection";
    m_selectorToCommand["moveWordLeft"] = "move_word_left";
    m_selectorToCommand["moveWordLeftAndModifySelection"] = "move_word_left_and_modify_selection";
    m_selectorToCommand["moveWordRight"] = "move_word_right";
    m_selectorToCommand["moveWordRightAndModifySelection"] = "move_word_right_and_modify_selection";
    m_selectorToCommand["pageDownAndModifySelection"] = "page_down_and_modify_selection";
    m_selectorToCommand["pageUpAndModifySelection"] = "page_up_and_modify_selection";
    m_selectorToCommand["scrollPageDown"] = "scroll_page_down";
    m_selectorToCommand["scrollPageUp"] = "scroll_page_up";
    m_selectorToCommand["scrollToBeginningOfDocument"] = "move_to_beginning_of_document";
    m_selectorToCommand["scrollToEndOfDocument"] = "move_to_end_of_document";
    m_selectorToCommand["transpose"] = "transpose";
    m_selectorToCommand["yank"] = "yank";
    m_selectorToCommand["redo"] = "redo";
    m_selectorToCommand["undo"] = "undo";
    m_selectorToCommand["selectAll"] = "select_all";
    m_selectorToCommand["cancelOperation"] = "cancel_operation";
}

std::shared_ptr<File> ContentView::getFile() const {
    return m_file;
}

int ContentView::getLines() {
    return m_dataSource->lines->height();
}

qreal ContentView::getMaxLineWidth() {
    return m_maxLineWidth;
}

int ContentView::getLinesHeight() {
    return getLines() * getLinespace();
}

int ContentView::getContentHeight() {
    return getLinesHeight() + m_padding.top();
}

int ContentView::getLinespace() {
    return m_dataSource->fontMetrics->height();
}

qreal ContentView::getMaxCharWidth() {
    return m_dataSource->fontMetrics->maxWidth();
}

qreal ContentView::getAverageCharWidth() {
    return m_dataSource->fontMetrics->averageCharWidth();
}

QPoint ContentView::getScrollOrigin() {
    return m_scrollOrigin;
}

int ContentView::getXOff() {
    return std::ceil(m_dataSource->gutterWidth + m_padding.left());
}

int ContentView::getLine(int y) {
    return qMax(0, (int)(m_scrollOrigin.y() + y - m_padding.top()) / getLinespace());
}

int ContentView::getColumn(int line, int x) {
    auto lineCache = m_dataSource->lines->locked();
    auto cacheLine = lineCache->get(line);
    if (!cacheLine) return -1;
    auto textline = cacheLine->assoc();
    if (!textline) return -1;
    return textline->xToIndex(m_scrollOrigin.x() + x);
}

int ContentView::checkLineVisible(int line) {
    auto viewRect = rect();
    auto fl = getVisibleLinesRange(viewRect);

    if (line < fl.first())
        return -1;
    else if (line > fl.last())
        return 1;
    else
        return 0;
}

qreal ContentView::getWidth(int lineIx, int columnIx) {
    auto lineCache = m_dataSource->lines->locked();
    auto line = lineCache->get(lineIx);
    if (line) {
        auto textLine = line->assoc();
        if (textLine) {
            return textLine->indexTox(columnIx);
        } else if (columnIx != 0) {
            return -1;
        }
    }
    return 0;
}

LineColumn ContentView::getLineColumn(const QPoint &pos) {
    auto line = getLine(pos.y());
    auto column = getColumn(line, pos.x());
    auto lineCache = m_dataSource->lines->locked();
    auto totalLines = lineCache->height();
    if (line >= totalLines) {
        line = totalLines - 1;
        column = lineCache->get(line)->utf8Length();
    }
    return LineColumn(line, column);
}

void ContentView::scrollY(int y) {
    auto value = y - m_padding.top();
    auto linespace = getLinespace();
    auto lines = getLines();
    if (lines == 0) return;

    constexpr auto kMaxPrefetch = 1;
    const int kLines = m_visibleLines * kMaxPrefetch;

    auto first = qMax(0, (int)(std::floor(value / qreal(linespace) + 0.9))); // last line [visible]
    first = qMin(lines - 1, first);
    if (m_firstLine != first) {
        m_firstLine = first;
        RangeI prefetch(qMax(0, m_firstLine - kLines), qMin(lines, m_firstLine + m_visibleLines + kLines));
        m_connection->sendScroll(m_file->viewId(), prefetch.start(), prefetch.end());
    }
    m_scrollOrigin.setY(m_firstLine * linespace);
    update();
    //repaint();
    //asyncPaint();
}

void ContentView::scrollX(int x) {
    m_scrollOrigin.setX(x);
    //repaint();
    update();
    //asyncPaint();
}

void ContentView::updateHandler(const QJsonObject &json) {
    QtConcurrent::run(QThreadPool::globalInstance(), [this, json]() {
        this->m_dataSource->lines->locked()->applyUpdate(json);
        emit repaintContentReceived();
    });
    repaint(); // update assoc
}

void ContentView::scrollHandler(int line, int column) {
    Q_UNUSED(line);
    Q_UNUSED(column);

    //repaint();
    update();
}

void ContentView::keyPressEvent(QKeyEvent *ev) {
    auto modifiers = ev->modifiers();
    auto ctrl = (modifiers & Qt::ControlModifier);
    auto alt = (modifiers & Qt::AltModifier);
    auto shift = (modifiers & Qt::ShiftModifier);
    auto meta = (modifiers & Qt::MetaModifier);
    auto none = (modifiers == Qt::NoModifier);

    switch (ev->key()) {
    case Qt::Key_Delete:
        deleteForward();
        return;
    case Qt::Key_Backspace:
        deleteBackward();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        insertNewline();
        return;
    case Qt::Key_Home:
        if (none)
            moveToBeginningOfLine();
        else if (ctrl && shift)
            moveToBeginningOfDocumentAndModifySelection();
        else if (shift)
            moveToBeginningOfLineAndModifySelection();
        else if (ctrl)
            moveToBeginningOfDocument();
        return;
    case Qt::Key_End:
        if (none)
            moveToEndOfLine();
        else if (ctrl && shift)
            moveToEndOfDocumentAndModifySelection();
        else if (shift)
            moveToEndOfLineAndModifySelection();
        else if (ctrl)
            moveToEndOfDocument();
        return;
    case Qt::Key_PageDown:
        if (none)
            scrollPageDown();
        else if (shift)
            pageDownAndModifySelection();
        return;
    case Qt::Key_PageUp:
        if (none)
            scrollPageUp();
        else if (shift)
            pageUpAndModifySelection();
        return;
    case Qt::Key_Up:
        if (none)
            moveUp();
        else if (shift)
            moveUpAndModifySelection();
        return;
    case Qt::Key_Down:
        if (none)
            moveDown();
        else if (shift)
            moveDownAndModifySelection();
        return;
    case Qt::Key_Left:
        if (none)
            moveLeft();
        else if (ctrl && shift)
            moveWordLeftAndModifySelection();
        else if (ctrl)
            moveWordLeft();
        else if (shift)
            moveLeftAndModifySelection();
        return;
    case Qt::Key_Right:
        if (none)
            moveRight();
        else if (ctrl && shift)
            moveWordRightAndModifySelection();
        else if (ctrl)
            moveWordRight();
        else if (shift)
            moveRightAndModifySelection();
        return;
    case Qt::Key_Z:
        if (ctrl) {
            undo();
            return;
        }
        break;
    case Qt::Key_Y:
        if (ctrl) {
            redo();
            return;
        }
        break;
    case Qt::Key_C:
        if (ctrl) {
            copy();
            return;
        }
        break;
    case Qt::Key_X:
        if (ctrl) {
            cut();
            return;
        }
        break;
    case Qt::Key_V:
        if (ctrl) {
            paste();
            return;
        }
        break;
    case Qt::Key_A:
        if (ctrl) {
            selectAll();
            return;
        }
        break;
    default:
        break;
    };

    QString text = ev->text();
    if (!text.isEmpty()) {
        insertChar(text);
    } else {
        QWidget::keyPressEvent(ev);
    }
}

QJsonObject clickGestureType(QMouseEvent *e, int count) {
    QJsonObject select;
    QJsonObject param;
    QString granularity;

    if (count == 3)
        granularity = "line";
    else if (count == 2)
        granularity = "word";
    else
        granularity = "point";

    auto modifiers = e->modifiers();
    auto ctrl = (modifiers & Qt::ControlModifier);
    auto shift = (modifiers & Qt::ShiftModifier);

    if (shift) {
        param["granularity"] = granularity;
        select["select_extend"] = param;
    } else {
        param["granularity"] = granularity;
        param["multi"] = ctrl ? true : false;
        select["select"] = param;
    }

    return select;
}

void ContentView::mousePressEvent(QMouseEvent *e) {
    if (m_mouseDoubleCheckTimer.remainingTime() > 0) {
        return;
    }

    setFocus();
    auto pos = e->pos();
    pos -= {getXOff(), 0};
    auto lc = getLineColumn(pos);
    if (lc.isValid()) {
        m_connection->sendGesture(m_file->viewId(), lc.line(), lc.column(), clickGestureType(e, 1));
        m_drag = true;
    }
    QWidget::mousePressEvent(e);
}

void ContentView::mouseMoveEvent(QMouseEvent *e) {
    if (m_drag) {
        setFocus();
        auto pos = e->pos();
        pos -= {getXOff(), 0};
        auto lc = getLineColumn(pos);
        if (lc.isValid())
            m_connection->sendDrag(m_file->viewId(), lc.line(), lc.column(), 0);
        // TODO:
        // if line == first || line == last
        // need auto scroll
    }
    QWidget::mouseMoveEvent(e);
}

void ContentView::mouseReleaseEvent(QMouseEvent *e) {
    setFocus();
    m_drag = false;
    QWidget::mouseReleaseEvent(e);
}

void ContentView::mouseDoubleClickEvent(QMouseEvent *e) {
    setFocus();
    auto pos = e->pos();
    pos -= {getXOff(), 0};
    auto lc = getLineColumn(pos);
    if (lc.isValid()) {
        m_mouseDoubleCheckTimer.setSingleShot(true);
        m_mouseDoubleCheckTimer.start(100);

        m_connection->sendGesture(m_file->viewId(), lc.line(), lc.column(), clickGestureType(e, 2));
    }
    QWidget::mouseDoubleClickEvent(e);
}

void ContentView::insertChar(const QString &text) {
    QJsonObject object;
    object["chars"] = text;
    m_connection->sendEdit(m_file->viewId(), "insert", object);
}

void ContentView::copy() {
    ResponseHandler handler([&](const QJsonObject &json) {
        QClipboard *clipboard = QApplication::clipboard();
        auto text = json["result"].toString();
        clipboard->setText(text);
    });
    m_connection->sendCopy(m_file->viewId(), handler);
}

void ContentView::cut() {
    ResponseHandler handler([&](const QJsonObject &json) {
        QClipboard *clipboard = QApplication::clipboard();
        auto text = json["result"].toString();
        clipboard->setText(text);
    });
    m_connection->sendCut(m_file->viewId(), handler);
}

void ContentView::paste() {
    QtConcurrent::run(QThreadPool::globalInstance(), [&]() {
        const QClipboard *clipboard = QApplication::clipboard();
        const QMimeData *mimeData = clipboard->mimeData();
        if (mimeData->hasText()) {
            auto text = mimeData->text();
            this->m_connection->sendPaste(this->m_file->viewId(), text);
        } else {
            qWarning() << "unknown clipboard data";
        }
    });
}

void ContentView::asyncPaint(int ms /*= 100*/) {
    Q_UNUSED(ms);

    using namespace std::chrono;
    auto timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    m_asyncPaintQueue.enqueue(timestamp);
}

void ContentView::themeChangedHandler() {
    //repaint();
    emit repaintContentReceived();
}

void ContentView::configChangedHandler(const QJsonObject &changes) {
    QtConcurrent::run(QThreadPool::globalInstance(), [=]() {
        m_dataSource->config->locked()->applyUpdate(changes);
    });
}

void ContentView::repaintContentHandler() {
    update();
    //repaint();
}

qreal ContentView::getAverageWidth(int line, int column) {
    Q_UNUSED(line);
    return getAverageCharWidth() * column;
}

qreal ContentView::getAscent() {
    return m_dataSource->fontMetrics->ascent();
}

QMarginsF ContentView::getPadding() {
    return m_padding;
}

void ContentView::tick() {
    auto editView = dynamic_cast<EditView *>(parent());
    editView->tick();
}

void ContentView::inputMethodEvent(QInputMethodEvent *event) {
    if (!event->commitString().isEmpty()) {
        auto text = event->commitString();
        insertChar(text);
        showImeComposition("");
    } else {
        showImeComposition(event->preeditString());
    }
}

void ContentView::showImeComposition(const QString &text) {
    m_imeComposition->setText(text);
    if (text.isEmpty()) {
        m_imeComposition->hide();
        return;
    }

    if (!m_imeComposition->isVisible()) {
        int h = m_dataSource->fontMetrics->height();
        m_imeComposition->setMinimumHeight(h);
        m_imeComposition->move(m_cursorCache.front());
        m_imeComposition->show();
    }

    m_imeComposition->setMinimumWidth(QFontMetricsF(m_imeComposition->font()).width(text));
    m_imeComposition->setMaximumWidth(QFontMetricsF(m_imeComposition->font()).width(text));
    m_imeComposition->update();
}

QVariant ContentView::inputMethodQuery(Qt::InputMethodQuery query) const {
    if (query == Qt::ImEnabled) {
        return true;
    } else if (query == Qt::ImFont) {
        return m_imeComposition->font();
    } else if (query == Qt::ImCursorRectangle) {
        if (!m_cursorCache.isEmpty()) {
            int h = m_dataSource->fontMetrics->height();
            auto pos = m_cursorCache.front();
            return QRect(pos, QSize(2, h));
        }
    }
    return QVariant();
}

AsyncPaintTimer::AsyncPaintTimer(QWidget *parent) {
    m_timer = std::make_unique<QTimer>(this);
    m_timer->start(10);
    connect(m_timer.get(), &QTimer::timeout, this, &AsyncPaintTimer::update);
    m_contentView = reinterpret_cast<ContentView *>(parent);
}

void AsyncPaintTimer::update() {
    if (m_contentView->m_asyncPaintQueue.size() != 0) {
        m_contentView->update();
        m_contentView->m_asyncPaintQueue.clear();
    }
}

DataSource::DataSource() {
    lines = std::make_shared<LineCache>();
    config = std::make_shared<Config>();
    {
        QString family = CONTENT_FONT;
        int size = 20;
        int weight = QFont::Normal;
        bool italic = false;
        QFont font(family, size, weight, italic);
        // PreferQuality PreferDefault PreferAntialias
        font.setStyleHint(QFont::Monospace, QFont::StyleStrategy(QFont::PreferAntialias | QFont::ForceIntegerMetrics));
        font.setFixedPitch(true);
        font.setKerning(false);
        defaultFont = std::make_shared<Font>(font);
    }
    fontMetrics = std::make_shared<QFontMetricsF>(defaultFont->getFont());
    gutterOne = fontMetrics->width('1');
    gutterWidth = gutterOne;
}

} // namespace xi
