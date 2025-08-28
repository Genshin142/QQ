#include "updatemanager.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QApplication>

UpdateManager::UpdateManager(QObject *parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &UpdateManager::onVersionCheckFinished);
}

void UpdateManager::setCurrentVersion(const QString &version)
{
    m_currentVersion = QVersionNumber::fromString(version);
}

void UpdateManager::setUpdateServerUrl(const QString &url)
{
    m_versionInfoUrl = QUrl(url);
}

void UpdateManager::checkForUpdates()
{
    if (m_versionInfoUrl.isEmpty()) {
        emit errorOccurred("更新服务器地址未设置");
        return;
    }

    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(m_versionInfoUrl));
    connect(reply, &QNetworkReply::errorOccurred, [this](QNetworkReply::NetworkError error) {
        emit errorOccurred(QString("网络错误: %1").arg(error));
    });
}

void UpdateManager::downloadUpdate()
{
    if (m_updatePackageUrl.isEmpty()) {
        emit errorOccurred("更新包地址未设置");
        return;
    }

    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(m_updatePackageUrl));
    connect(reply, &QNetworkReply::downloadProgress,
            this, &UpdateManager::downloadProgress);
    connect(reply, &QNetworkReply::finished,
            this, &UpdateManager::onUpdateDownloaded);
    connect(reply, &QNetworkReply::errorOccurred, [this](QNetworkReply::NetworkError error) {
        emit errorOccurred(QString("下载错误: %1").arg(error));
    });
}

void UpdateManager::installUpdate()
{
    if (m_downloadedFilePath.isEmpty()) {
        emit errorOccurred("没有可安装的更新包");
        return;
    }

    // 根据不同平台处理安装逻辑
#ifdef Q_OS_WIN
    // Windows通常使用安装程序
    QProcess::startDetached(m_downloadedFilePath);
    qApp->quit();
#elif defined(Q_OS_MAC)
    // macOS可以使用pkg安装包
    QProcess::startDetached("open", QStringList() << m_downloadedFilePath);
    qApp->quit();
#elif defined(Q_OS_LINUX)
    // Linux可能使用deb或rpm包
    QProcess::startDetached("xdg-open", QStringList() << m_downloadedFilePath);
    qApp->quit();
#endif
}

void UpdateManager::onVersionCheckFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        m_latestVersion = obj["version"].toString();
        m_releaseNotes = obj["releaseNotes"].toString();
        m_updatePackageUrl = QUrl(obj["downloadUrl"].toString());

        QVersionNumber latestVersion = QVersionNumber::fromString(m_latestVersion);

        emit updateCheckFinished(latestVersion > m_currentVersion,
                                 m_latestVersion,
                                 m_releaseNotes);
    } else {
        emit errorOccurred(QString("检查更新失败: %1").arg(reply->errorString()));
    }

    reply->deleteLater();
}

void UpdateManager::onUpdateDownloaded()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        // 保存下载的文件
        QString tempDir = QDir::tempPath();
        QString fileName = m_updatePackageUrl.fileName();
        m_downloadedFilePath = tempDir + QDir::separator() + fileName;

        QFile file(m_downloadedFilePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            emit downloadFinished(m_downloadedFilePath);
        } else {
            emit errorOccurred("无法保存更新包");
        }
    } else {
        emit errorOccurred(QString("下载失败: %1").arg(reply->errorString()));
    }

    reply->deleteLater();
}
