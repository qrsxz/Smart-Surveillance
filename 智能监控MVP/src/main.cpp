#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.setWindowTitle("智能监控 MVP - 运动检测");
    w.resize(800, 500);
    w.show();

    return app.exec();
}
