#include <QCoreApplication>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "backendconfig.h"
#include "backendservice.h"

namespace {

struct ParsedArguments {
    QHash<QString, QString> options;
    QStringList positionals;
};

void printUsage()
{
    QTextStream out(stdout);
    out << "Backend command runner\n\n"
        << "Commands:\n"
        << "  start                                 # start long-running backend service\n"
        << "  configure <endpoint> <projectId> <apiKey> <databaseId>\n"
        << "            [messagesCollection] [membersCollection] [channelsCollection] [sessionsCollection]\n"
        << "            [--guest-access <true|false>]\n"
        << "  users list\n"
        << "  users add <email> <password> [name]\n"
        << "  users remove <userId>\n"
        << "  collections list\n"
        << "  collections add <collectionId> [name]\n"
        << "  collections remove <collectionId>\n"
        << "  channels list\n"
        << "  channels add <name>\n"
        << "  channels remove <channelId>\n"
        << "  members list <channelId>\n"
        << "  members add <channelId> <userId> [displayName]\n"
        << "  members remove <channelId> <userId>\n"
        << "  sessions list\n"
        << "  sessions add <channelId> <userId> [userId ...]\n"
        << "  sessions close <sessionId>\n"
        << "  messages list <channelId> [limit]\n"
        << "  messages find <channelId> <messageId> [limit]\n"
        << "  messages add <channelId> <senderId> <payloadJsonOrText>\n"
        << "  messages remove <messageId>\n"
        << "  serve | echo-service                  # aliases of start\n";
}

void printJsonObject(const QJsonObject &obj)
{
    QTextStream out(stdout);
    out << QJsonDocument(obj).toJson(QJsonDocument::Indented);
}

void printJsonArray(const QJsonArray &arr)
{
    QTextStream out(stdout);
    out << QJsonDocument(arr).toJson(QJsonDocument::Indented);
}

void printError(const QString &message)
{
    QTextStream err(stderr);
    err << message << Qt::endl;
}

bool parseArguments(const QStringList &rawArgs, ParsedArguments *parsed, QString *errorMessage)
{
    if (!parsed) {
        if (errorMessage) {
            *errorMessage = "Internal error: parsed output pointer is null";
        }
        return false;
    }

    ParsedArguments result;
    for (int index = 0; index < rawArgs.size(); ++index) {
        const QString token = rawArgs.at(index);
        if (!token.startsWith("--")) {
            result.positionals.append(token);
            continue;
        }

        const QString optionName = token.mid(2).trimmed();
        if (optionName.isEmpty()) {
            if (errorMessage) {
                *errorMessage = "Invalid empty option name";
            }
            return false;
        }

        if ((index + 1) >= rawArgs.size() || rawArgs.at(index + 1).startsWith("--")) {
            if (errorMessage) {
                *errorMessage = QString("Missing value for option --%1").arg(optionName);
            }
            return false;
        }

        result.options.insert(optionName, rawArgs.at(index + 1));
        ++index;
    }

    *parsed = result;
    return true;
}

bool ensureNoOptions(const ParsedArguments &parsed, QString *errorMessage)
{
    if (!parsed.options.isEmpty()) {
        const QString optionName = parsed.options.constBegin().key();
        if (errorMessage) {
            *errorMessage = QString("Unexpected option --%1 for this command").arg(optionName);
        }
        return false;
    }
    return true;
}

bool parseBooleanOptionValue(const QString &value, bool *result, QString *errorMessage)
{
    if (!result) {
        if (errorMessage) {
            *errorMessage = "Internal error: boolean output pointer is null";
        }
        return false;
    }
    const QString normalized = value.trimmed().toLower();
    if (normalized == "true" || normalized == "1" || normalized == "yes" || normalized == "on") {
        *result = true;
        return true;
    }
    if (normalized == "false" || normalized == "0" || normalized == "no" || normalized == "off") {
        *result = false;
        return true;
    }
    if (errorMessage) {
        *errorMessage = QString("Invalid boolean value '%1'. Use true/false").arg(value);
    }
    return false;
}

bool parseConfigureOptions(const ParsedArguments &parsed, bool *guestAccessEnabled, QString *errorMessage)
{
    if (!guestAccessEnabled) {
        if (errorMessage) {
            *errorMessage = "Internal error: guest access output pointer is null";
        }
        return false;
    }

    bool value = false;
    bool valueSet = false;
    const auto parseGuestOption = [&](const QString &optionName) -> bool {
        if (!parsed.options.contains(optionName)) {
            return true;
        }
        if (valueSet) {
            if (errorMessage) {
                *errorMessage = "Specify only one of --guest-access or --guestAccess";
            }
            return false;
        }
        valueSet = true;
        return parseBooleanOptionValue(parsed.options.value(optionName), &value, errorMessage);
    };

    if (!parseGuestOption("guest-access") || !parseGuestOption("guestAccess")) {
        return false;
    }

    for (auto it = parsed.options.constBegin(); it != parsed.options.constEnd(); ++it) {
        if (it.key() != "guest-access" && it.key() != "guestAccess") {
            if (errorMessage) {
                *errorMessage = QString("Unexpected option --%1 for configure command").arg(it.key());
            }
            return false;
        }
    }

    *guestAccessEnabled = valueSet ? value : false;
    return true;
}

QString joinPositionals(const QStringList &positionals, int startIndex)
{
    if (startIndex >= positionals.size()) {
        return QString();
    }
    return positionals.mid(startIndex).join(" ").trimmed();
}

}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setApplicationName("backend");

    const QStringList arguments = QCoreApplication::arguments();
    QString command;
    QStringList rawCommandArguments;
    if (arguments.size() < 2) {
        command = "start";
    } else {
        command = arguments.at(1).trimmed();
        rawCommandArguments = arguments.mid(2);
    }

    if (command == "help" || command == "--help" || command == "-h") {
        printUsage();
        return 0;
    }

    ParsedArguments parsed;
    QString parseError;
    if (!parseArguments(rawCommandArguments, &parsed, &parseError)) {
        printError(parseError);
        return 1;
    }

    const bool isConfigureCommand =
        (command == "configure"
         || command == "config");

    if (isConfigureCommand) {
        bool guestAccessEnabled = false;
        QString configureOptionsError;
        if (!parseConfigureOptions(parsed, &guestAccessEnabled, &configureOptionsError)) {
            printError(configureOptionsError);
            return 1;
        }
        if (parsed.positionals.size() < 4 || parsed.positionals.size() > 8) {
            printError("Usage: backend configure <endpoint> <projectId> <apiKey> <databaseId> [messagesCollection] [membersCollection] [channelsCollection] [sessionsCollection] [--guest-access <true|false>]");
            return 1;
        }

        backend::BackendConfig config;
        config.endpoint = parsed.positionals.at(0).trimmed();
        config.projectId = parsed.positionals.at(1).trimmed();
        config.apiKey = parsed.positionals.at(2).trimmed();
        config.databaseId = parsed.positionals.at(3).trimmed();
        config.guestAccessEnabled = guestAccessEnabled;

        if (parsed.positionals.size() >= 5) {
            config.messagesCollectionId = parsed.positionals.at(4).trimmed();
        }
        if (parsed.positionals.size() >= 6) {
            config.membersCollectionId = parsed.positionals.at(5).trimmed();
        }
        if (parsed.positionals.size() >= 7) {
            config.channelsCollectionId = parsed.positionals.at(6).trimmed();
        }
        if (parsed.positionals.size() >= 8) {
            config.sessionsCollectionId = parsed.positionals.at(7).trimmed();
        }

        if (!config.isValid()) {
            printError("Invalid configuration: one or more required fields are missing");
            return 1;
        }

        const QString requestedDatabaseId = config.databaseId;
        QString configSaveError;
        if (!backend::saveConfig(config, &configSaveError)) {
            printError(configSaveError);
            return 1;
        }

        backend::BackendService service(config);
        QString bootstrapError;
        if (!service.bootstrap(&bootstrapError)) {
            printError(bootstrapError);
            return 1;
        }

        const backend::BackendConfig finalConfig = service.resolvedConfig();
        if (!backend::saveConfig(finalConfig, &configSaveError)) {
            printError(configSaveError);
            return 1;
        }

        QJsonObject output;
        output["status"] = "configured";
        output["configPath"] = backend::defaultConfigPath();
        output["databaseId"] = finalConfig.databaseId;
        output["requestedDatabaseId"] = requestedDatabaseId;
        output["usedExistingDatabase"] = (finalConfig.databaseId != requestedDatabaseId);
        output["guestAccessEnabled"] = finalConfig.guestAccessEnabled;
        printJsonObject(output);
        return 0;
    }

    backend::BackendConfig config;
    QString loadError;
    if (!backend::loadConfig(&config, &loadError)) {
        printError(loadError + ". Run `backend configure ...` first.");
        return 1;
    }

    backend::BackendService service(config);
    QString error;

    const bool requiresBootstrap =
        (command == "start"
         || command == "serve"
         || command == "echo-service"
         || command == "collections"
         || command == "channels"
         || command == "members"
         || command == "sessions"
         || command == "messages");

    if (requiresBootstrap) {
        QString bootstrapError;
        if (!service.bootstrap(&bootstrapError)) {
            printError(bootstrapError);
            return 1;
        }
    }

    if (command == "start" || command == "serve" || command == "echo-service") {
        if (!ensureNoOptions(parsed, &error)) {
            printError(error);
            return 1;
        }
        if (!parsed.positionals.isEmpty()) {
            printError("start command does not accept positional arguments");
            return 1;
        }

        const int serviceExitCode = service.runEchoService(&error);
        if (serviceExitCode != 0 && !error.isEmpty()) {
            printError(error);
        }
        return serviceExitCode;
    }

    if (command == "users") {
        if (!ensureNoOptions(parsed, &error)) {
            printError(error);
            return 1;
        }
        if (parsed.positionals.isEmpty()) {
            printError("users command requires an action: list | add | remove");
            return 1;
        }

        const QString action = parsed.positionals.at(0).trimmed().toLower();
        if (action == "list") {
            if (parsed.positionals.size() != 1) {
                printError("Usage: backend users list");
                return 1;
            }
            QJsonArray users;
            if (!service.listUsers(&users, &error)) {
                printError(error);
                return 1;
            }
            printJsonArray(users);
            return 0;
        }

        if (action == "add") {
            if (parsed.positionals.size() < 3) {
                printError("Usage: backend users add <email> <password> [name]");
                return 1;
            }
            const QString email = parsed.positionals.at(1).trimmed();
            const QString password = parsed.positionals.at(2);
            const QString name = joinPositionals(parsed.positionals, 3);

            QJsonObject createdUser;
            if (!service.createUser(email, password, name, &createdUser, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(createdUser);
            return 0;
        }

        if (action == "remove") {
            if (parsed.positionals.size() != 2) {
                printError("Usage: backend users remove <userId>");
                return 1;
            }
            const QString userId = parsed.positionals.at(1).trimmed();
            if (!service.deleteUser(userId, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(QJsonObject{{"status", "deleted"}, {"userId", userId}});
            return 0;
        }

        printError(QString("Unknown users action: %1").arg(action));
        return 1;
    }

    if (command == "collections") {
        if (!ensureNoOptions(parsed, &error)) {
            printError(error);
            return 1;
        }
        if (parsed.positionals.isEmpty()) {
            printError("collections command requires an action: list | add | remove");
            return 1;
        }

        const QString action = parsed.positionals.at(0).trimmed().toLower();
        if (action == "list") {
            if (parsed.positionals.size() != 1) {
                printError("Usage: backend collections list");
                return 1;
            }
            QJsonArray collections;
            if (!service.listCollections(&collections, &error)) {
                printError(error);
                return 1;
            }
            printJsonArray(collections);
            return 0;
        }

        if (action == "add") {
            if (parsed.positionals.size() < 2) {
                printError("Usage: backend collections add <collectionId> [name]");
                return 1;
            }
            const QString collectionId = parsed.positionals.at(1).trimmed();
            const QString name = joinPositionals(parsed.positionals, 2);

            QJsonObject collection;
            if (!service.addCollection(collectionId, name, &collection, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(collection);
            return 0;
        }

        if (action == "remove") {
            if (parsed.positionals.size() != 2) {
                printError("Usage: backend collections remove <collectionId>");
                return 1;
            }
            const QString collectionId = parsed.positionals.at(1).trimmed();
            if (!service.removeCollection(collectionId, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(QJsonObject{{"status", "removed"}, {"collectionId", collectionId}});
            return 0;
        }

        printError(QString("Unknown collections action: %1").arg(action));
        return 1;
    }

    if (command == "channels") {
        if (!ensureNoOptions(parsed, &error)) {
            printError(error);
            return 1;
        }
        if (parsed.positionals.isEmpty()) {
            printError("channels command requires an action: list | add | remove");
            return 1;
        }

        const QString action = parsed.positionals.at(0).trimmed().toLower();
        if (action == "list") {
            if (parsed.positionals.size() != 1) {
                printError("Usage: backend channels list");
                return 1;
            }
            QJsonArray channels;
            if (!service.listChannels(&channels, &error)) {
                printError(error);
                return 1;
            }
            printJsonArray(channels);
            return 0;
        }

        if (action == "add") {
            if (parsed.positionals.size() < 2) {
                printError("Usage: backend channels add <name>");
                return 1;
            }
            const QString name = joinPositionals(parsed.positionals, 1);
            QJsonObject channel;
            if (!service.createChannel(name, &channel, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(channel);
            return 0;
        }

        if (action == "remove") {
            if (parsed.positionals.size() != 2) {
                printError("Usage: backend channels remove <channelId>");
                return 1;
            }
            const QString channelId = parsed.positionals.at(1).trimmed();
            if (!service.removeChannel(channelId, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(QJsonObject{{"status", "removed"}, {"channelId", channelId}});
            return 0;
        }

        printError(QString("Unknown channels action: %1").arg(action));
        return 1;
    }

    if (command == "members") {
        if (!ensureNoOptions(parsed, &error)) {
            printError(error);
            return 1;
        }
        if (parsed.positionals.isEmpty()) {
            printError("members command requires an action: list | add | remove");
            return 1;
        }

        const QString action = parsed.positionals.at(0).trimmed().toLower();
        if (action == "list") {
            if (parsed.positionals.size() != 2) {
                printError("Usage: backend members list <channelId>");
                return 1;
            }
            const QString channelId = parsed.positionals.at(1).trimmed();
            QJsonArray members;
            if (!service.listMembers(channelId, &members, &error)) {
                printError(error);
                return 1;
            }
            printJsonArray(members);
            return 0;
        }

        if (action == "add") {
            if (parsed.positionals.size() < 3) {
                printError("Usage: backend members add <channelId> <userId> [displayName]");
                return 1;
            }
            const QString channelId = parsed.positionals.at(1).trimmed();
            const QString userId = parsed.positionals.at(2).trimmed();
            const QString displayName = joinPositionals(parsed.positionals, 3);
            QJsonObject member;
            if (!service.addMember(channelId, userId, displayName, &member, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(member);
            return 0;
        }

        if (action == "remove") {
            if (parsed.positionals.size() != 3) {
                printError("Usage: backend members remove <channelId> <userId>");
                return 1;
            }
            const QString channelId = parsed.positionals.at(1).trimmed();
            const QString userId = parsed.positionals.at(2).trimmed();
            if (!service.removeMember(channelId, userId, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(QJsonObject{
                {"status", "removed"},
                {"channelId", channelId},
                {"userId", userId}
            });
            return 0;
        }

        printError(QString("Unknown members action: %1").arg(action));
        return 1;
    }

    if (command == "messages") {
        if (!ensureNoOptions(parsed, &error)) {
            printError(error);
            return 1;
        }
        if (parsed.positionals.isEmpty()) {
            printError("messages command requires an action: list | find | add | remove");
            return 1;
        }

        const QString action = parsed.positionals.at(0).trimmed().toLower();
        if (action == "list") {
            if (parsed.positionals.size() < 2 || parsed.positionals.size() > 3) {
                printError("Usage: backend messages list <channelId> [limit]");
                return 1;
            }
            const QString channelId = parsed.positionals.at(1).trimmed();
            int limit = 100;
            if (parsed.positionals.size() == 3) {
                bool ok = false;
                limit = parsed.positionals.at(2).toInt(&ok);
                if (!ok || limit <= 0) {
                    printError("Limit must be a positive integer");
                    return 1;
                }
            }
            QJsonArray messages;
            if (!service.readMessages(channelId, QString(), limit, &messages, &error)) {
                printError(error);
                return 1;
            }
            printJsonArray(messages);
            return 0;
        }

        if (action == "find") {
            if (parsed.positionals.size() < 3 || parsed.positionals.size() > 4) {
                printError("Usage: backend messages find <channelId> <messageId> [limit]");
                return 1;
            }
            const QString channelId = parsed.positionals.at(1).trimmed();
            const QString messageId = parsed.positionals.at(2).trimmed();
            int limit = 100;
            if (parsed.positionals.size() == 4) {
                bool ok = false;
                limit = parsed.positionals.at(3).toInt(&ok);
                if (!ok || limit <= 0) {
                    printError("Limit must be a positive integer");
                    return 1;
                }
            }
            QJsonArray messages;
            if (!service.readMessages(channelId, messageId, limit, &messages, &error)) {
                printError(error);
                return 1;
            }
            printJsonArray(messages);
            return 0;
        }

        if (action == "add") {
            if (parsed.positionals.size() < 4) {
                printError("Usage: backend messages add <channelId> <senderId> <payloadJsonOrText>");
                return 1;
            }
            const QString channelId = parsed.positionals.at(1).trimmed();
            const QString senderId = parsed.positionals.at(2).trimmed();
            const QString payloadInput = joinPositionals(parsed.positionals, 3);

            QJsonObject payload;
            QJsonParseError payloadParseError;
            const QJsonDocument payloadDocument =
                QJsonDocument::fromJson(payloadInput.toUtf8(), &payloadParseError);
            if (payloadParseError.error == QJsonParseError::NoError && payloadDocument.isObject()) {
                payload = payloadDocument.object();
            } else {
                payload["text"] = payloadInput;
            }

            QJsonObject message;
            if (!service.createMessage(channelId, senderId, payload, &message, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(message);
            return 0;
        }

        if (action == "remove") {
            if (parsed.positionals.size() != 2) {
                printError("Usage: backend messages remove <messageId>");
                return 1;
            }
            const QString messageId = parsed.positionals.at(1).trimmed();
            if (!service.removeMessage(messageId, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(QJsonObject{{"status", "removed"}, {"messageId", messageId}});
            return 0;
        }

        printError(QString("Unknown messages action: %1").arg(action));
        return 1;
    }

    if (command == "sessions") {
        if (!ensureNoOptions(parsed, &error)) {
            printError(error);
            return 1;
        }
        if (parsed.positionals.isEmpty()) {
            printError("sessions command requires an action: list | add | close");
            return 1;
        }

        const QString action = parsed.positionals.at(0).trimmed().toLower();
        if (action == "list") {
            if (parsed.positionals.size() != 1) {
                printError("Usage: backend sessions list");
                return 1;
            }
            QJsonArray sessions;
            if (!service.listSessions(&sessions, &error)) {
                printError(error);
                return 1;
            }
            printJsonArray(sessions);
            return 0;
        }

        if (action == "add") {
            if (parsed.positionals.size() < 3) {
                printError("Usage: backend sessions add <channelId> <userId> [userId ...]");
                return 1;
            }
            const QString channelId = parsed.positionals.at(1).trimmed();
            QStringList userIds;
            for (int i = 2; i < parsed.positionals.size(); ++i) {
                const QString userId = parsed.positionals.at(i).trimmed();
                if (!userId.isEmpty()) {
                    userIds.append(userId);
                }
            }
            if (userIds.isEmpty()) {
                printError("At least one non-empty userId is required");
                return 1;
            }

            QJsonObject createdSession;
            if (!service.createSession(channelId, userIds, &createdSession, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(createdSession);
            return 0;
        }

        if (action == "close") {
            if (parsed.positionals.size() != 2) {
                printError("Usage: backend sessions close <sessionId>");
                return 1;
            }
            const QString sessionId = parsed.positionals.at(1).trimmed();
            if (!service.closeSession(sessionId, &error)) {
                printError(error);
                return 1;
            }
            printJsonObject(QJsonObject{{"status", "closed"}, {"sessionId", sessionId}});
            return 0;
        }

        printError(QString("Unknown sessions action: %1").arg(action));
        return 1;
    }

    printError(QString("Unknown command: %1").arg(command));
    printUsage();
    return 1;
}
