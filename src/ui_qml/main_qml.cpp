#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include "Backend.h"
#include "SliceImageProvider.h"
#include "../core/NiiVolumeLoader.h"

int main(int argc, char* argv[]) {
    NiiVolumeLoader::RegisterFactories();

    QGuiApplication app(argc, argv);


    QQmlApplicationEngine engine;

    auto* imageProvider = new SliceImageProvider(); // 所有权交给engine管理

    Backend backend(imageProvider);

    engine.addImageProvider("sliceprovider", imageProvider);

    // 用单例(singleton)方式暴露给QML，比context property更可靠、时序问题更少
    qmlRegisterSingletonInstance("TumorSegUi", 1, 0, "Backend", &backend);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                      &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("TumorSegUi", "Main");

    return app.exec();
}