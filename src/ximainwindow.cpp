#include "ximainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QGridLayout>
#include <QStandardPaths>
#include <QTabBar>
#include <QtGlobal>

namespace xi {

static constexpr const char *XI_CONFIG_DIR = "XI_CONFIG_DIR";
static constexpr const char *XI_PLUGINS = "plugins";
static constexpr const char *XI_THEME = "InspiredGitHub"; // "base16-eighties.dark" // "InspiredGitHub"

XiMainWindow::XiMainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    setupCore();
    setupEditWindow();
    setupStartPage();
}

XiMainWindow::~XiMainWindow() {
    m_coreConnection.reset();
}

void XiMainWindow::keyPressEvent(QKeyEvent *e) {
    QMainWindow::keyPressEvent(e);
}

void XiMainWindow::setupUI() {
    setWindowIcon(QIcon(":/resources/icons/xi-editor.ico"));
    setWindowTitle("xi-qt");

    m_editWindow = new EditWindow();
    m_editWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_editWindow->tabBar()->setFocusPolicy(Qt::NoFocus);

    m_stack.insertWidget(1, m_editWindow);
    m_stack.setCurrentIndex(1);

    setCentralWidget(&m_stack);
}

void XiMainWindow::setupCore() {
    m_coreConnection = std::make_shared<CoreConnection>();
    m_coreConnection->init();

    QString configDir = qEnvironmentVariable(XI_CONFIG_DIR);
    if (configDir.isEmpty())
        configDir = defaultConfigDirectory();
    QDir dir = QCoreApplication::applicationDirPath();
    if (!dir.cd(XI_PLUGINS)) {
        dir.mkdir(XI_PLUGINS);
        dir.cd(XI_PLUGINS);
    }
    QString clientExtrasDir = dir.absolutePath();
    m_coreConnection->sendClientStarted(configDir, clientExtrasDir);

    //default theme
    m_coreConnection->sendSetTheme(XI_THEME);
}

void XiMainWindow::setupEditWindow() {
    m_editWindow->init(m_coreConnection);
}

void XiMainWindow::setupStartPage() {
}

QString XiMainWindow::defaultConfigDirectory() {
    return QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation).front();
}

void XiMainWindow::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event);

    // notify save

    // exit core process
     m_coreConnection->uninit();
}


} // namespace xi