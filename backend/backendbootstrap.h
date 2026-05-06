#ifndef BACKENDBOOTSTRAP_H
#define BACKENDBOOTSTRAP_H

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

#include <functional>

#include "appwritesdk.h"
#include "backendconfig.h"

namespace backend {

struct BackendRequestResult {
    bool completed = false;
    bool success = false;
    QJsonObject data;
    int code = 0;
    QString message;
};

class BackendBootstrapper {
public:
    using RequestRunner = std::function<BackendRequestResult(const std::function<void()> &, int)>;

    BackendBootstrapper(const BackendConfig &config,
                        appwritesdk::Server *server,
                        RequestRunner requestRunner);

    BackendConfig resolvedConfig() const;
    bool bootstrap(QString *errorMessage);

private:
    struct AttributeDef {
        QString type;
        QString key;
        bool required = true;
        QJsonObject options;
    };

    struct IndexDef {
        QString key;
        QString type;
        QStringList attributes;
    };

    static QString toError(const QString &operation, const BackendRequestResult &result);
    bool isAlreadyExists(const BackendRequestResult &result) const;
    bool isBootstrapLimitError(const BackendRequestResult &result) const;
    bool shouldRetryIndexCreation(const BackendRequestResult &result) const;
    bool isDatabaseLimitReached(const BackendRequestResult &result) const;
    bool canUseRequestedDatabase(QString *errorMessage);
    bool adoptExistingDatabase(QString *errorMessage);
    appwritesdk::ConnectionConfig configForCollection(const QString &collectionId) const;
    bool ensureCollection(const QString &collectionId,
                          const QString &name,
                          const QJsonArray &permissions,
                          const QList<AttributeDef> &attributes,
                          const QList<IndexDef> &indexes,
                          QString *errorMessage);

    BackendConfig m_config;
    appwritesdk::Server *m_server = nullptr;
    RequestRunner m_requestRunner;
};

} // namespace backend

#endif // BACKENDBOOTSTRAP_H
