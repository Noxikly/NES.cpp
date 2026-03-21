#include <QApplication>

#include "gui/w_main.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QString romPath;
    if (argc > 1)
        romPath = QString::fromLocal8Bit(argv[1]);
        
    WMain mainWindow(romPath);
    mainWindow.show();

    return app.exec();
}
