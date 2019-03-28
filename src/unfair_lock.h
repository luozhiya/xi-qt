#ifndef UNFAIR_LOCK_H
#define UNFAIR_LOCK_H

#include <QMutex>

#include <memory>

namespace xi {

class UnfairLock {
public:
    UnfairLock() {
        m_lock.reset(new QMutex());
    }

    inline void lock() {
        m_lock->lock();
    }

    inline void unlock() {
        m_lock->unlock();
    }

    inline bool tryLock() {
        return m_lock->tryLock();
    }

protected:
    std::unique_ptr<QMutex> m_lock;
};

} // namespace xi

#endif // UNFAIR_LOCK_H