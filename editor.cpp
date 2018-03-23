#include "editor.h"

#include <QInputMethodEvent>

namespace xi {

Line::Line()
{

}

Line::Line(const QJsonObject &value)
{
    fromJson(value);
}

void Line::fromJson(const QJsonObject &value)
{
    m_cursors.clear();
    m_text.clear();

    m_text = value["text"].toString();
    auto arr = value["cursor"].toArray();
    for (auto c : arr) {
        m_cursors.push_back(c.toInt());
    }
}

QString Line::text()
{
    return m_text;
}

QVector<int> Line::cursors()
{
    return m_cursors;
}

int LineCache::height()
{
    return m_lines.size();
}

const Line &LineCache::getLine(int idx)
{
    return m_lines.at(idx);
}

void LineCache::applyUpdate(const QJsonObject& value)
{
    QVector<Line> lines;
    int old_idx = 0;

    auto ops_arr = value["ops"].toArray();
    for (auto op : ops_arr) {
        auto op_type_str = op.toObject()["op"].toString();
        int n = op.toObject()["n"].toInt();
        auto op_type = opType(op_type_str);
        switch (op_type) {
        case OP_COPY:
            for (int i = 0; i < n; ++i) {
                lines.push_back(m_lines.at(i+old_idx));
            }
            old_idx += n;
            break;
        case OP_SKIP:
            old_idx += n;
            break;
        case OP_INVALIDATE:
            for (int i = 0; i < n; ++i) {
                lines.push_back(Line());
            }
            old_idx += n;
            break;
        case OP_UPDATE:

            break;
        case OP_INS:
        {
            auto lines_arr = op.toObject()["lines"].toArray();
            for (auto line : lines_arr) {
                lines.push_back(Line(line.toObject()));
            }
            break;
        }
        default:
            qDebug() << op_type_str;
            break;
        }
    }
    m_lines = lines;
}

const QMap<LineCache::OpType, QString> LineCache::m_opmap = {
    {OP_COPY, "copy"},
    {OP_SKIP, "skip"},
    {OP_INVALIDATE, "invalidate"},
    {OP_UPDATE, "update"},
    {OP_INS, "ins"},
};

LineCache::OpType LineCache::opType(QString name)
{
    return m_opmap.key(name, OP_UNKOWN);
}

Editor::Editor(XiBridge *bridge, const EditorOption& option, QWidget *parent)
    :QWidget(parent), m_bridge(bridge), m_option(option)
{

}

void Editor::init()
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_KeyCompression, false);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);

    setAttribute(Qt::WA_InputMethodEnabled, true);
    m_imeComposition = new QLabel(this);
    m_imeComposition->setVisible(false);
    m_imeComposition->setTextFormat(Qt::PlainText);
    m_imeComposition->setTextInteractionFlags(Qt::NoTextInteraction);
    m_imeComposition->setAutoFillBackground(true);

    setOption(m_option);
    connect(m_bridge, SIGNAL(jsonAvailable(const QJsonObject&)), this, SLOT(updateEditorCache(const QJsonObject&)));
    new_view();
}

void Editor::new_view(QString file)
{
    m_bridge->new_view(file);
}

void Editor::setOption(const EditorOption &option)
{
    m_option = option;
    setFont(m_option.font);
    emit editorOptionChanged();
}

EditorOption Editor::getDefaultOption()
{
    QString family = "Consolas";
    int size = 12;
    int weight  = -1;
    bool italic = false;
    QFont font(family, size, weight, italic);
    font.setStyleHint(QFont::TypeWriter, QFont::StyleStrategy(QFont::PreferDefault | QFont::ForceIntegerMetrics));
    font.setFixedPitch(true);
    font.setKerning(false);

    EditorOption option;
    option.font = font;
    option.bg = QColor(0x272822);
    option.fg = QColor(0xf0f0ea);
    return option;
}

QString Editor::getViewId()
{
    return m_viewId;
}

void Editor::setViewId(QString id)
{
    m_viewId = id;
}

QVariant Editor::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (query == Qt::ImEnabled) {
        return true;
    } else if (query == Qt::ImFont){
        return font();
    } else if (query == Qt::ImCursorRectangle) {
        QFontMetrics fm(m_option.font);
        int h = fm.height();
        auto pos = cursorPos();
        return QRect(pos, QSize(1, h));
    }
    return QVariant();
}

QPoint Editor::cursorPos(int ix) const
{
    return m_cursors.at(ix);
}

void Editor::updateEditorCache(const QJsonObject& data)
{
    if (data["result"].isString()) {
        m_viewId = data["result"].toString();
    }else {
        auto method = data["method"].toString();
        if (method.compare("update") == 0) {
            auto update = data["params"].toObject()["update"].toObject();
            m_cache.applyUpdate(update);
        }
    }
    update();
}

void Editor::paintEvent(QPaintEvent *ev)
{
    Q_UNUSED(ev);

    m_cursors.clear();
    QPainter painter(this);

    auto full = rect();
    painter.fillRect(full, m_option.bg);

    QFontMetrics fm(m_option.font);
    painter.setPen(m_option.fg);

    int h = fm.height();
    int x = 6;
    int y = h+2;
    int lineSpace = 1;
    for (int num = 0; num < m_cache.height(); ++num) {
        auto line = m_cache.getLine(num);
        auto text = line.text();
        painter.drawText(x, y-1, text);
        // draw cursors
        auto cursors = line.cursors();
        for (auto offset: cursors) {
            auto u8text = text.toUtf8();
            auto sub = u8text.left(offset);
            int w = fm.width(sub);
            QRect rect(w+x, y-17, 1, h);
            m_cursors.push_back(QPoint(w+x, y-17));
            auto mode = painter.compositionMode();
            painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
            painter.fillRect(rect, m_option.fg);
            painter.setCompositionMode(mode);
        }
        y+=h + lineSpace;
    }
}

void Editor::keyPressEvent(QKeyEvent *ev)
{
    switch (ev->key()) {
    case Qt::Key_Delete:
        m_bridge->delete_forward();
        return;
    case Qt::Key_Backspace:
        m_bridge->delete_backward();
        return;
    case Qt::Key_Return:
        m_bridge->insert_newline();
        return;
    case Qt::Key_Up:
        m_bridge->move_up();
        return;
    case Qt::Key_Down:
        m_bridge->move_down();
        return;
    case Qt::Key_Left:
        m_bridge->move_left();
        return;
    case Qt::Key_Right:
        m_bridge->move_right();
        return;
    default:
        break;
    };

    QString text = ev->text();
    if (!text.isEmpty()) {
        m_bridge->insert(text);
    }else {
        QWidget::keyPressEvent(ev);
    }
}

void Editor::inputMethodEvent(QInputMethodEvent *event)
{
    if (!event->commitString().isEmpty())
    {
        auto text = event->commitString();
        m_bridge->insert(text);
        showImeComposition("");
    } else {
        showImeComposition(event->preeditString());
    }
}

void Editor::showImeComposition(const QString& text)
{
    m_imeComposition->setText(text);
    if (text.isEmpty()) {
        m_imeComposition->hide();
        return;
    }

    if (!m_imeComposition->isVisible()) {
        QFontMetrics fm(m_option.font);
        int h = fm.height();

        m_imeComposition->setMinimumHeight(h);
        m_imeComposition->move(cursorPos());
        m_imeComposition->show();
    }

    m_imeComposition->setMinimumWidth(QFontMetrics(m_imeComposition->font()).width(text));
    m_imeComposition->setMaximumWidth(QFontMetrics(m_imeComposition->font()).width(text));
    m_imeComposition->update();
}

}

