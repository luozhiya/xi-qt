#include "ximainwindow.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QSurfaceFormat>

int main(int argc, char *argv[]) {
    Q_INIT_RESOURCE(xi);

    QSurfaceFormat format;
    format.setSamples(4);
    format.setSwapInterval(0); // disable vsync
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    xi::XiMainWindow w;
    w.show();
    w.resize(800, 600);
    //w.resize(1366, 768);
    w.move((QApplication::desktop()->width() - w.width()) / 2, (QApplication::desktop()->height() - w.height()) / 2);

    return a.exec();
}
