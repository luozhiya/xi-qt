#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QKeySequence>
#include <QShortcut>
#include <QWidget>

#include <functional>

namespace xi {

class Shortcuts {
    using Callback = std::function<void(QShortcut *)>;

public:
    Shortcuts(const Shortcuts &other) = delete;
    Shortcuts &operator=(const Shortcuts &other) = delete;
    ~Shortcuts();

    static Shortcuts *shared();
    void append(QWidget *parent, const QKeySequence &seq, Callback callback);
    void erase(QWidget *widget);

private:
    Shortcuts() {}
};

class HasShortcuts {
public:
    explicit HasShortcuts(QWidget *toErase) : m_toErase(toErase) {}
    ~HasShortcuts();

protected:
    QWidget *m_toErase;
};

} // namespace xi

#endif // SHORTCUTS_H