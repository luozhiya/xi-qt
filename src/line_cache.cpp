#include "line_cache.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "core_connection.h"

namespace xi {

enum OpsType {
    Ops_Invalidate = 0,
    Ops_Ins,
    Ops_Copy,
    Ops_Update,
    Ops_Skip,
    Ops_Unknown,
};

static QHash<QString, OpsType> opsMap;

static OpsType to_ops(const QString &name) {
    if (opsMap.size() == 0) {
        opsMap["invalidate"] = Ops_Invalidate;
        opsMap["ins"] = Ops_Ins;
        opsMap["copy"] = Ops_Copy;
        opsMap["update"] = Ops_Update;
        opsMap["skip"] = Ops_Skip;
    }
    return opsMap.value(name, Ops_Unknown);
}

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
}

Line &Line::operator=(const Line &line) {
    if (this != &line) {
        m_text = line.m_text;
        m_cursor = line.m_cursor;
        m_styles = line.m_styles;
        m_assoc = line.m_assoc;
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
    for (auto opref : ops) {
        auto op = opref.toObject();
        auto opTypeStr = op["op"].toString();
        //qDebug() << "update--> op type: " << opTypeStr;
        auto opType = to_ops(opTypeStr);
        auto n = op["n"].toInt();
        switch (opType) {
        case Ops_Invalidate: {
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
        case Ops_Ins: {
            for (int i = 0; i < newInvalidAfter; ++i) {
                newLines.push_back(nullptr);
            }
            newInvalidAfter = 0;
            inval.addRangeN(newInvalidBefore + newLines.count(), n);
            QJsonArray jsonLines = op["lines"].toArray();
            for (auto jsonLine : jsonLines) {
                newLines.push_back(std::make_shared<Line>(jsonLine.toObject()));
            }
        } break;
        case Ops_Copy:
        case Ops_Update: {
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
                if (oldIdx != newInvalidBefore + newLines.size() || opType != Ops_Copy) {
                    inval.addRangeN(newInvalidBefore + newLines.count(), nCopy);
                }
                auto startIx = oldIdx - m_invalidBefore;
                if (opType == Ops_Copy) {
                    for (auto i = startIx; i < startIx + nCopy; ++i) {
                        newLines.push_back(std::move(m_lines[i]));
                    }
                } else { // Ops_Update
                    QJsonArray jsonLines = op["lines"].toArray();
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
        case Ops_Skip:
            oldIdx += n;
            break;
        default:
            qDebug() << "unknown op type " << opTypeStr;
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
