#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>

#include "democontroller.h"
#include "../appcomm/model.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    appcomm::model::AppCommConfig config;
    config.endpoint = "https://fra.cloud.appwrite.io/v1";
    config.projectId = "69a007d7000a5a976062";
    config.databaseId = "default";
    config.messagesCollectionId = "messages";
    config.incomingMessagesCollectionId = "pendingmessages";
    config.membersCollectionId = "members";

    QQmlApplicationEngine engine;
    DemoController controller(config);

    qmlRegisterSingletonInstance("App", 1, 0, "AppController", &controller);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("frontend", "Main");

    return app.exec();
}