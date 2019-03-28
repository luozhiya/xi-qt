#ifndef RANGE_H
#define RANGE_H

#include <QPair>

namespace xi {

template <typename T>
class Range {
public:
    Range(T start = 0, T end = 0) {
        m_pair.first = start;
        m_pair.second = end;
    }
    inline T start() const {
        return m_pair.first;
    }
    void start(T start) {
        m_pair.first = start;
    }
    inline T end() const {
        return m_pair.second;
    }
    void end(T end) {
        m_pair.second = end;
    }
    inline T size() const {
        return m_pair.second - m_pair.first;
    }
    inline T length() const {
        return size();
    }
    inline bool isEmpty() const {
        return m_pair.second == m_pair.first;
    }

protected:
    QPair<T, T> m_pair;
};

using RangeI = Range<int>;
using RangeF = Range<qreal>;

template <typename T>
class ClosedRange {
public:
    ClosedRange(T first = 0, T last = 0) {
        m_pair.first = first;
        m_pair.second = last;
    }
    inline T first() const {
        return m_pair.first;
    }
    void first(T first) {
        m_pair.first = first;
    }
    inline T last() const {
        return m_pair.second;
    }
    void last(T last) {
        m_pair.second = last;
    }
    inline T size() const {
        return m_pair.second - m_pair.first + 1;
    }
    inline T length() const {
        return size();
    }
    inline bool isEmpty() const {
        return m_pair.second < m_pair.first;
    }

protected:
    QPair<T, T> m_pair;
};

using ClosedRangeI = ClosedRange<int>;
using ClosedRangeF = ClosedRange<qreal>;

} // namespace xi

#endif // RANGE_H
