#ifndef STYLE_MAP_H
#define STYLE_MAP_H

#include <QColor>
#include <QDebug>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include <memory>

#include "range.h"
#include "style.h"
#include "style_span.h"
#include "text_line.h"
#include "unfair_lock.h"

namespace xi {

QColor colorFromArgb(quint32 argb);

class StyleMapState : public UnfairLock {
public:
    void defStyle(const QJsonObject &json);
    void applyStyle(std::shared_ptr<TextLineBuilder> builder, int id, const RangeI &range, const QColor &selColor);
    void applyStyles(std::shared_ptr<TextLineBuilder> builder, std::shared_ptr<QList<StyleSpan>> styles, const QColor &selColor, const QColor &highlightColor);    

private:
    QList<std::shared_ptr<Style>> m_styles;
};

class StyleMapLocked {
public:
    StyleMapLocked(std::shared_ptr<StyleMapState> mutex) {
        m_inner = mutex;
        m_inner->lock();
    }

    ~StyleMapLocked() {
        m_inner->unlock();
    }

    inline void defStyle(const QJsonObject &json) {
        m_inner->defStyle(json);
    }

    inline void applyStyle(std::shared_ptr<TextLineBuilder> builder, int id, const RangeI &range, const QColor &selColor) {
        m_inner->applyStyle(builder, id, range, selColor);
    }

    inline void applyStyles(std::shared_ptr<TextLineBuilder> builder,
                     std::shared_ptr<QList<StyleSpan>> styles,
                     const QColor &selColor, const QColor &highlightColor) {
        m_inner->applyStyles(builder, styles, selColor, highlightColor);
    }

private:
    std::shared_ptr<StyleMapState> m_inner;
};

class StyleMap {
public:
    StyleMap(){
        m_state = std::make_shared<StyleMapState>();
    }

    inline std::shared_ptr<StyleMapLocked> locked() {
        return std::make_shared<StyleMapLocked>(m_state);
    }

private:
    std::shared_ptr<StyleMapState> m_state;
};

} // namespace xi

#endif // STYLE_MAP_H
