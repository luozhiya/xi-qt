#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QTabBar>
#include <QtConcurrent>
#include <QThreadPool>

#include <string>

#include "edit_view.h"
#include "edit_window.h"
#include "perference.h"
#include "shortcuts.h"
#include "style_map.h"

namespace xi {

static const char *UNTITLED_NAME = "untitled";

EditWindow::EditWindow(QWidget *parent) : QTabWidget(parent) {
    setContentsMargins(0, 0, 0, 0);

    setupShortcuts();
    connect(this, &EditWindow::newViewIdRecevied, this, &EditWindow::newViewIdHandler);
}

EditWindow::~EditWindow() {

}

void EditWindow::init(const std::shared_ptr<CoreConnection> &connection) {
    m_connection = connection;
    setupCoreHandler();
    newTab();
}

void EditWindow::openFile(const QString &viewId, const QString &filePath) {
    auto idx = find(viewId, filePath);
    if (idx != -1) {
        setCurrentIndex(idx);
        EditView *view = tab(idx);
        view->focusOnEdit();
        return;
    }
    ResponseHandler handler([this, filePath](const QJsonObject &result) {
        auto newViewId = result["result"].toString();
        emit this->newViewIdRecevied(newViewId, filePath);
    });
    m_connection->sendNewView(filePath, handler);
}

void EditWindow::openFile() {
    QStringList filePathList;
    std::unique_ptr<QFileDialog> dlg = std::make_unique<QFileDialog>();
    dlg->resize(410, 470);
    dlg->setViewMode(QFileDialog::List);
    if (dlg->exec() == QDialog::Accepted) {
        filePathList = dlg->selectedFiles();
    } else {
        return;
    }
    openFile(QString(), filePathList.front());
}

void EditWindow::closeFile(const QString &viewId) {
    auto idx = find(viewId, QString());
    if (idx != -1) {
        removeViewTab(idx);
        m_router.erase(m_router.find(viewId));
        m_connection->sendCloseView(viewId);
    }
}

void EditWindow::saveFile(bool saveAs) {
    auto idx = currentIndex();
    auto view = tab(idx);
    auto file = view->getFile();
    if (file->path().isEmpty() || saveAs) {
        std::unique_ptr<QFileDialog> dlg = std::make_unique<QFileDialog>();
        dlg->setAcceptMode(QFileDialog::AcceptMode::AcceptSave);
        dlg->resize(410, 470);
        dlg->setViewMode(QFileDialog::List);
        dlg->selectFile(file->displayName());
        if (dlg->exec() == QDialog::Accepted) {
            file->setPath(dlg->selectedFiles().front());
        } else {
            return;
        }
    }
    m_connection->sendSave(file->viewId(), file->path());
    setTabText(idx, file->name());
    setTabToolTip(idx, file->path());
}

void EditWindow::saveAsFile() {
    saveFile(true);
}

int EditWindow::find(const QString &viewId, const QString &filePath) {
    for (int i = 0; i < count(); ++i) {
        if ((!viewId.isEmpty() && tab(i)->getFile()->viewId() == viewId) ||
            (viewId.isEmpty() && !filePath.isEmpty() && tab(i)->getFile()->path() == filePath)) {
            return i;
        }
    }
    return -1;
}

EditView *EditWindow::tab(int idx) {
    return dynamic_cast<EditView *>(widget(idx));
}

int EditWindow::insertViewTab(int currIdx, const std::shared_ptr<File> &file, QWidget *view) {
    auto fileName = file->name();

    int idx = insertTab(currIdx, view, fileName);
    if (idx == -1) return -1;

    if (fileName.isEmpty()) {
        fileName = UNTITLED_NAME;
        file->setTempName(UNTITLED_NAME);
    }
    setTabText(idx, fileName);

    setTabToolTip(idx, file->path());

    // file type detected.
    //setTabIcon(idx, );

    return idx;
}

int EditWindow::appendViewTab(const std::shared_ptr<File> &file, QWidget *view) {
    return insertViewTab(count() + 1, file, view);
}

void EditWindow::removeViewTab(int idx) {
    if (idx > -1 && idx < tabBar()->count()) {
        EditView *view = tab(idx);
        removeTab(idx);
        delete view;
    }
}

void EditWindow::resizeEvent(QResizeEvent *event) {
    QTabWidget::resizeEvent(event);
}

void EditWindow::keyPressEvent(QKeyEvent *e) {
    QTabWidget::keyPressEvent(e);
}

void EditWindow::setupShortcuts() {
    {
        auto seq = QKeySequence("Ctrl+O");
        Shortcuts::shared()->append(this, seq, [&](QShortcut *shortcut) {
            shortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(shortcut, &QShortcut::activated,
                    this, &EditWindow::newTabWithOpenFile);
        });
    }
    {
        auto seq = QKeySequence("Ctrl+N");
        Shortcuts::shared()->append(this, seq, [&](QShortcut *shortcut) {
            shortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(shortcut, &QShortcut::activated,
                    this, &EditWindow::newTab);
        });
    }
    {
        auto seq = QKeySequence("Ctrl+W");
        Shortcuts::shared()->append(this, seq, [&](QShortcut *shortcut) {
            shortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(shortcut, &QShortcut::activated,
                    this, &EditWindow::closeCurrentTab);
        });
    }
    {
        auto seq = QKeySequence("Ctrl+S");
        Shortcuts::shared()->append(this, seq, [&](QShortcut *shortcut) {
            shortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(shortcut, &QShortcut::activated,
                    this, &EditWindow::saveCurrentTab);
        });
    }
}

void EditWindow::setupCoreHandler() {
    connect(m_connection.get(), &CoreConnection::updateReceived, this, &EditWindow::updateHandler);
    connect(m_connection.get(), &CoreConnection::scrollReceived, this, &EditWindow::scrollHandler);
    connect(m_connection.get(), &CoreConnection::pluginStartedReceived, this, &EditWindow::pluginStartedHandler);
    connect(m_connection.get(), &CoreConnection::pluginStoppedReceived, this, &EditWindow::pluginStoppedHandler);
    connect(m_connection.get(), &CoreConnection::availablePluginsReceived, this, &EditWindow::availablePluginsHandler);
    connect(m_connection.get(), &CoreConnection::updateCommandsReceived, this, &EditWindow::updateCommandsHandler);
    connect(m_connection.get(), &CoreConnection::configChangedReceived, this, &EditWindow::configChangedHandler);
    connect(m_connection.get(), &CoreConnection::defineStyleReceived, this, &EditWindow::defineStyleHandler);
    connect(m_connection.get(), &CoreConnection::availableThemesReceived, this, &EditWindow::availableThemesHandler);
    connect(m_connection.get(), &CoreConnection::themeChangedReceived, this, &EditWindow::themeChangedHandler);
    connect(m_connection.get(), &CoreConnection::alertReceived, this, &EditWindow::alertHandler);
}

void EditWindow::newTabWithOpenFile() {
    openFile();
}

void EditWindow::newTab() {
    openFile(QString(), QString());
}

void EditWindow::closeCurrentTab() {
    auto idx = currentIndex();
    if (idx != -1) closeFile(tab(idx)->getFile()->viewId());
}

void EditWindow::saveCurrentTab() {
    if (currentIndex() != -1) saveFile();
}

void EditWindow::saveAllTab() {
}

void EditWindow::updateHandler(const QString &viewId, const QJsonObject &update) {
    auto view = dynamic_cast<EditView *>(m_router[viewId]);
    if (view) view->updateHandler(update);
}

void EditWindow::scrollHandler(const QString &viewId, int line, int column) {
    auto view = dynamic_cast<EditView *>(m_router[viewId]);
    if (view) view->scrollHandler(line, column);
}

void EditWindow::pluginStartedHandler(const QString &viewId, const QString &pluginName) {
    Q_UNUSED(viewId);
    Q_UNUSED(pluginName);
}

void EditWindow::pluginStoppedHandler(const QString &viewId, const QString &pluginName) {
    Q_UNUSED(viewId);
    Q_UNUSED(pluginName);
}

void EditWindow::availablePluginsHandler(const QString &viewId, const QJsonObject &plugins) {
    Q_UNUSED(viewId);
    Q_UNUSED(plugins);
    //auto view = dynamic_cast<EditView *>(m_router[viewId]);
    //if (view) view->availablePluginsHandler();
}

void EditWindow::updateCommandsHandler(const QString &viewId, const QStringList &commands) {
    Q_UNUSED(viewId);
    Q_UNUSED(commands);
}

void EditWindow::configChangedHandler(const QString &viewId, const QJsonObject &changes) {
    auto view = dynamic_cast<EditView *>(m_router[viewId]);
    if (view) view->configChangedHandler(changes);
}

void EditWindow::defineStyleHandler(const QJsonObject &json) {
    QtConcurrent::run(QThreadPool::globalInstance(), [json]() {
        Perference::shared()->styleMap()->locked()->defStyle(json);
    });
}

void EditWindow::availableThemesHandler(const QStringList &themes) {
    Q_UNUSED(themes);
}

void EditWindow::themeChangedHandler(const QString &name, const QJsonObject &json) {
    QtConcurrent::run(QThreadPool::globalInstance(), [this, name, json]() {
        //qDebug() << "themeChangedHandler";
        Perference::shared()->theme()->locked()->applyUpdate(name, json);
        auto i = this->m_router.constBegin();
        while (i != this->m_router.constEnd()) {
            auto viewId = i.key();
            auto view = dynamic_cast<EditView *>(i.value());
            if (view) view->themeChangedHandler();
            ++i;
        }
    });
}

void EditWindow::alertHandler(const QString &text) {
    QMessageBox msgBox;
    msgBox.setText(text);
    msgBox.exec();
}

void EditWindow::newViewIdHandler(const QString &newViewId, const QString &filePath) {
    auto file = std::make_shared<File>();
    file->setPath(filePath);
    file->setViewId(newViewId);
    EditView *view = new EditView(file, this->m_connection, this);
    auto newIdx = this->appendViewTab(file, view);
    if (newIdx == -1) {
        qDebug() << "insert tab failed";
        delete view;
    } else {
        this->m_router[newViewId] = view;
        setCurrentIndex(newIdx);
        view->focusOnEdit();
    }
}

} // namespace xi
