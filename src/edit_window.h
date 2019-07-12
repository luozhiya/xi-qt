#ifndef EDIT_WINDOW_H
#define EDIT_WINDOW_H

#include <QHash>
#include <QTabWidget>

#include <memory>

#include "core_connection.h"
#include "file.h"

namespace xi {

class EditView;

// Tab
class EditWindow : public QTabWidget {
    Q_OBJECT
public:
    explicit EditWindow(QWidget *parent = nullptr);
    ~EditWindow();

    void init(const std::shared_ptr<CoreConnection> &connection);

    void openFile(const QString &viewId, const QString &filePath);
    void openFile();
    void closeFile(const QString &viewId);
    void saveFile(bool saveAs = false);
    void saveAsFile();

    int insertViewTab(int idx, const std::shared_ptr<File> &file, QWidget *view);
    int appendViewTab(const std::shared_ptr<File> &file, QWidget *view);
    void removeViewTab(int idx);

    int find(const QString &viewId, const QString &filePath);

    inline EditView *tab(int idx);
    void newTab();
    void newTabWithOpenFile();
    void closeCurrentTab();
    void saveCurrentTab();
    void saveAllTab();

    void setupShortcuts();
    void addShortcut(QString sequence, void(xi::EditWindow::*functionToCall)());
    void setupCoreHandler();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *e) override;

signals:
    void newViewIdRecevied(const QString &viewId, const QString &file);

public slots:
    void updateHandler(const QString &viewId, const QJsonObject &update);
    void scrollHandler(const QString &viewId, int line, int column);
    void pluginStartedHandler(const QString &viewId, const QString &pluginName);
    void pluginStoppedHandler(const QString &viewId, const QString &pluginName);
    void availablePluginsHandler(const QString &viewId, const QJsonObject &plugins);
    void updateCommandsHandler(const QString &viewId, const QStringList &commands);
    void configChangedHandler(const QString &viewId, const QJsonObject &changes);
    void defineStyleHandler(const QJsonObject &params);
    void availableThemesHandler(const QStringList &themes);
    void themeChangedHandler(const QString &name, const QJsonObject &json);
    void alertHandler(const QString &text);

    void newViewIdHandler(const QString &viewId, const QString &file);

private:
    std::shared_ptr<CoreConnection> m_connection;
    QHash<QString, QWidget *> m_router;
};

} // namespace xi

#endif // EDIT_WINDOW_H
