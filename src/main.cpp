#include <QApplication>
#include <QCoreApplication>

#include "gui/w_main.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QString romPath;
    const QStringList args = QCoreApplication::arguments();
    if (args.size() > 1)
        romPath = args.at(1);

    WMain mainWindow(romPath);
    mainWindow.show();

    return app.exec();
}
