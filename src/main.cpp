#include <QApplication>
#include "core/NiiVolumeLoader.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    NiiVolumeLoader::RegisterFactories();

    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}

