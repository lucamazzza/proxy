#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDebug>
#include <QStringList>
#include <QTextStream>

#include "democontroller.h"
#include "frontendconfig.h"
#include "../appcomm/model.h"

namespace {

void writeInfo(const QString &message)
{
    QTextStream(stdout) << message << Qt::endl;
}

void writeError(const QString &message)
{
    QTextStream(stderr) << message << Qt::endl;
}

void printUsage()
{
    const QString usage =
        "Frontend demo\n\n"
        "Commands:\n"
        "  configure <endpoint> <projectId> <databaseId> "
        "[messagesCollection] [membersCollection] [incomingMessagesCollection]\n"
        "  config path\n"
        "  help\n\n"
        "Example:\n"
        "  appfrontend configure https://fra.cloud.appwrite.io/v1 "
        "<projectId> <databaseId> messages members pendingmessages\n\n"
        "Parameters:\n"
        "  endpoint: Appwrite endpoint ending with /v1\n"
        "  projectId: Appwrite project ID\n"
        "  databaseId: Appwrite database ID\n"
        "  messagesCollection: messages collection ID, default: messages\n"
        "  membersCollection: members collection ID, default: members\n"
        "  incomingMessagesCollection: pending messages collection ID, default: pendingmessages\n\n"
        "Without a command, the GUI starts using the frontend config.";

    writeInfo(usage);
}

bool configureFrontend(const QStringList &args)
{
    if (args.size() < 5 || args.size() > 8) {
        const QString usage =
            "Usage: appfrontend configure <endpoint> <projectId> <databaseId> "
            "[messagesCollection] [membersCollection] [incomingMessagesCollection]\n"
            "Example: appfrontend configure https://fra.cloud.appwrite.io/v1 "
            "<projectId> <databaseId> messages members pendingmessages";

        writeError(usage);
        return false;
    }

    appcomm::model::AppCommConfig config;
    config.endpoint = args.at(2).trimmed();
    config.projectId = args.at(3).trimmed();
    config.databaseId = args.at(4).trimmed();
    config.messagesCollectionId = args.size() >= 6 ? args.at(5).trimmed() : "messages";
    config.membersCollectionId = args.size() >= 7 ? args.at(6).trimmed() : "members";
    config.incomingMessagesCollectionId = args.size() >= 8 ? args.at(7).trimmed() : "pendingmessages";

    QString saveError;
    if (!frontend::saveFrontendConfig(config, &saveError)) {
        writeError(saveError);
        return false;
    }

    const QString confirmation =
        QString("Frontend config written: %1\n"
                "endpoint=%2\n"
                "projectId=%3\n"
                "databaseId=%4\n"
                "messagesCollectionId=%5\n"
                "membersCollectionId=%6\n"
                "incomingMessagesCollectionId=%7")
            .arg(frontend::defaultFrontendConfigPath(),
                 config.endpoint,
                 config.projectId,
                 config.databaseId,
                 config.messagesCollectionId,
                 config.membersCollectionId,
                 config.incomingMessagesCollectionId);

    writeInfo(confirmation);
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QStringList rawArguments;
    for (int i = 0; i < argc; ++i) {
        rawArguments.append(QString::fromLocal8Bit(argv[i]));
    }

    if (rawArguments.size() >= 2) {
        QCoreApplication commandApp(argc, argv);

        const QString command = rawArguments.at(1).trimmed().toLower();
        if (command == "help" || command == "--help" || command == "-h") {
            printUsage();
            return 0;
        }
        if (command == "configure" || command == "config") {
            if (command == "config" && rawArguments.size() == 3 && rawArguments.at(2) == "path") {
                writeInfo(frontend::defaultFrontendConfigPath());
                return 0;
            }
            return configureFrontend(rawArguments) ? 0 : 1;
        }
    }

    QGuiApplication app(argc, argv);

    appcomm::model::AppCommConfig config;
    QString configError;
    if (!frontend::loadFrontendConfig(&config, &configError)) {
        qCritical().noquote()
            << "Cannot start frontend:" << configError
            << "\nCreate" << frontend::defaultFrontendConfigPath()
            << "with `appfrontend configure ...` first.";
        return 1;
    }

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
