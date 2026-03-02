/*!
 * @file appwritesdk.h
 * @brief Short description...
 *
 * Full description...
 *
 * @copyright
 */

#ifndef APPWRITESDK_H
#define APPWRITESDK_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

/*!
 * @brief Short description...
 *
 * Full description...
 */
namespace AppwriteSDK {

/*!
 * @brief Short description...
 *
 * Full description...
 */
struct ConnectionConfig {
    QString endpoint;
    QString projectId;
    QString apiKey;
    QString dbId;
    QString collectionId;
};

/*!
 * @brief Short description...
 *
 * Full description...
 */
class BaseSDK : public QObject {
    Q_OBJECT
public:

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    explicit BaseSDK(QNetworkAccessManager *mgr, QObject *parent = nullptr);

signals:

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    void requestSuccess(const QJsonObject &data);

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    void requestError(int code, const QString &message);

protected slots:

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    void onResponseFinished();

protected:

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    QNetworkAccessManager *m_network;

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    QNetworkRequest createBaseRequest(const ConnectionConfig &config, const QString &path, bool isAdmin);
};

/*!
 * @brief Short description...
 *
 * Full description...
 */
class Client : public BaseSDK {
    Q_OBJECT
public:
    using BaseSDK::BaseSDK;

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    void createAnonymousSession(const ConnectionConfig &config);

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    void createEmailSession(const ConnectionConfig &config, const QString &email, const QString &password);

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    void createDocument(const ConnectionConfig &config, const QJsonObject &data);
};

/*!
 * @brief Short description...
 *
 * Full description...
 */
class Server : public BaseSDK {
    Q_OBJECT
public:
    using BaseSDK::BaseSDK;

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    void createDatabase(const ConnectionConfig &config, const QString &name);

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    void createCollection(const ConnectionConfig &config, const QString &name);

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    void createStringAttribute(const ConnectionConfig &config, const QString &key, int size, bool required);

    /*!
     * @brief Short description...
     *
     * Full description...
     */
    void createUser(const ConnectionConfig &config, const QString &email, const QString &password);
};

} // namespace AppwriteSDK

#endif // APPWRITESDK_H
