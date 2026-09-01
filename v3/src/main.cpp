#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.setWindowTitle(QStringLiteral("智能监控 v3 - 九宫格多路监控"));
    w.resize(1280, 760);
    w.show();

    return app.exec();
}
