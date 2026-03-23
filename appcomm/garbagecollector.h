#ifndef GARBAGECOLLECTOR_H
#define GARBAGECOLLECTOR_H

#include <QObject>

#include "appwritesdk.h"
#include "model.h"

namespace appcomm {

namespace server {

/*!
 * @brief
 *
 *
 */
class GarbageCollector : public QObject {
    Q_OBJECT
public:

    /*!
     * @brief
     *
     * @param server
     * @param config
     * @param parent
     */
    explicit GarbageCollector(appwritesdk::Server *server,
                              const appwritesdk::ConnectionConfig &config,
                              QObject *parent = nullptr);

    /*!
     * @brief runCleanup
     *
     * @param policy
     */
    void runCleanup(const model::PersistencePolicy &policy);

signals:

    /*!
     * @brief
     *
     * @param deletedCount
     */
    void cleanupComplete(int deletedCount);

    /*!
     * @brief
     *
     * @param error
     */
    void cleanupError(const QString &error);

private slots:

    /*!
     * @brief
     *
     * @param data
     */
    void onDocumentsListed(const QJsonObject &data);

    /*!
     * @brief
     *
     * @param data
     */
    void onDocumentDeleted(const QJsonObject &data);

    /*!
     * @brief
     *
     * @param code
     * @param message
     */
    void onError(int code, const QString &message);

private:
    appwritesdk::Server *m_server;          ///<
    appwritesdk::ConnectionConfig m_config; ///<
    QStringList m_docsToDelete;             ///<
    int m_deletedCount;                     ///<
};

} // namespace server

} // namespace appcomm

#endif // GARBAGECOLLECTOR_H
