#include "line_cache.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "core_connection.h"

#include "base.h"

namespace xi {

enum class Op {
    invalidate,
    ins,
    copy,
    update,
    skip,
    unknown,
};

Line::Line(const QJsonObject &json) {
    m_assoc = nullptr;
    m_styles = std::make_shared<QList<StyleSpan>>();
    m_cursor = std::make_shared<QList<int>>();

    if (json["text"].isString()) {
        m_text = json["text"].toString();
        if (json.contains("cursor")) {
            auto jsonCursors = json["cursor"].toArray();
            for (auto jsonCursor : jsonCursors) {
                m_cursor->append(jsonCursor.toInt());
            }
        }
        if (json.contains("styles")) {
            auto jsonStyles = json["styles"].toArray();
            m_styles = StyleSpan::styles(jsonStyles, m_text);
        } else {
            m_styles->clear();
        }
        if (json.contains("ln")) {
            auto jsonNumber = json["ln"];
            if (jsonNumber.isNull()) {
                // soft break
            } else {
                m_number = jsonNumber.toInt();
            }
        }
    }
}

Line::Line(std::shared_ptr<Line> line, const QJsonObject &json) {
    m_assoc = nullptr;    
    if (!line) { return; }
    m_text = line->m_text;
    if (json.contains("cursor")) {
        m_cursor = std::make_shared<QList<int>>();
        auto jsonCursors = json["cursor"].toArray();
        for (auto jsonCursor : jsonCursors) {
            m_cursor->append(jsonCursor.toInt());
        }
    } else {
        m_cursor = line->m_cursor;
    }
    if (json.contains("styles")) {
        auto jsonStyles = json["styles"].toArray();
        m_styles = StyleSpan::styles(jsonStyles, m_text);
    } else {
        m_styles = line->m_styles;
    }
    if (json.contains("ln")) {
        auto jsonNumber = json["ln"];
        if (jsonNumber.isNull()) {
            // soft break
        } else {
            m_number = jsonNumber.toInt();
        }
    } else {
        m_number = line->m_number;
    }
}

Line &Line::operator=(const Line &line) {
    if (this != &line) {
        m_text = line.m_text;
        m_cursor = line.m_cursor;
        m_styles = line.m_styles;
        m_assoc = line.m_assoc;
        m_number = line.m_number;
    }
    return *this;
}

InvalSet LineCacheState::applyUpdate(const QJsonObject &json) {
    InvalSet inval;
    if (!json.contains("ops") || !json["ops"].isArray()) {
        return inval;
    }

    auto oldHeight = height();
    int newInvalidBefore = 0;
    int newInvalidAfter = 0;
    int oldIdx = 0;
    CacheLines newLines;

    auto ops = json["ops"].toArray();
    for (auto opRef : ops) {
        auto opObj = opRef.toObject();
        auto opStr = opObj["op"].toString();
        auto op = to_enum(opStr, Op::unknown);
        auto n = opObj["n"].toInt();
        switch (op) {
        case Op::invalidate: {
            auto curLine = newInvalidBefore + newLines.size() + newInvalidAfter;
            auto ix = curLine - m_invalidBefore;
            if (ix + n > 0 && ix < m_lines.size()) {
                for (auto i = qMax(ix, 0); i < qMin(ix + n, m_lines.size()); ++i) {
                    if (m_lines[i] != nullptr) {
                        inval.addRangeN(i + newInvalidBefore, 1);
                    }
                }
            }
            if (newLines.size() == 0) {
                newInvalidBefore += n;
            } else {
                newInvalidAfter += n;
            }
        } break;
        case Op::ins: {
            for (int i = 0; i < newInvalidAfter; ++i) {
                newLines.push_back(nullptr);
            }
            newInvalidAfter = 0;
            inval.addRangeN(newInvalidBefore + newLines.count(), n);
            QJsonArray jsonLines = opObj["lines"].toArray();
            for (auto jsonLine : jsonLines) {
                newLines.push_back(std::make_shared<Line>(jsonLine.toObject()));
            }
        } break;
        case Op::copy:
        case Op::update: {
            auto nRemaining = n;
            if (oldIdx < m_invalidBefore) {
                auto nInvalid = qMin(n, m_invalidBefore - oldIdx);
                if (newLines.size() == 0) {
                    newInvalidBefore += nInvalid;
                } else {
                    newInvalidAfter += nInvalid;
                }
                oldIdx += nInvalid;
                nRemaining -= nInvalid;
            }
            if (nRemaining > 0 && oldIdx < m_invalidBefore + m_lines.size()) {
                for (int i = 0; i < newInvalidAfter; ++i) {
                    newLines.push_back(nullptr);
                }
                newInvalidAfter = 0;
                auto nCopy = qMin(nRemaining, m_invalidBefore + (int)m_lines.size() - oldIdx);
                if (oldIdx != newInvalidBefore + newLines.size() || op != Op::copy) {
                    inval.addRangeN(newInvalidBefore + newLines.count(), nCopy);
                }
                auto startIx = oldIdx - m_invalidBefore;
                if (op == Op::copy) {
                    auto ln = opObj["ln"].toInt();
                    auto lineNumber = ln;
                    for (auto i = startIx; i < startIx + nCopy; ++i) {
                        if (m_lines[i]) {
                            m_lines[i]->setNumber(lineNumber);
                            lineNumber += 1;
                        }
                        newLines.push_back(std::move(m_lines[i]));
                    }
                } else { // Op::update
                    QJsonArray jsonLines = opObj["lines"].toArray();
                    auto jsonIx = n - nRemaining;
                    for (auto ix = startIx; ix < startIx + nCopy; ++ix) {
                        newLines.push_back(std::make_shared<Line>(m_lines[ix], jsonLines[ix].toObject()));
                        jsonIx += 1;
                    }
                }
                oldIdx += nCopy;
                nRemaining -= nCopy;
            }
            if (newLines.size() == 0) {
                newInvalidBefore += nRemaining;
            } else {
                newInvalidAfter += nRemaining;
            }
            oldIdx += nRemaining;
        } break;
        case Op::skip:
            oldIdx += n;
            break;
        default:
            qDebug() << "unknown op type " << opStr;
            break;
        }
    }

    m_invalidBefore = newInvalidBefore;
    m_lines = newLines;
    m_invalidAfter = newInvalidAfter;
    m_revision++;

    if (height() < oldHeight) {
        inval.addRange(height(), oldHeight);
    }
    return inval;
}

} // namespace xi
