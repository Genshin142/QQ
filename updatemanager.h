#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QVersionNumber>

class UpdateManager : public QObject
{
    Q_OBJECT
public:
    explicit UpdateManager(QObject *parent = nullptr);

    // 设置当前版本和检查更新的服务器地址
    void setCurrentVersion(const QString &version);
    void setUpdateServerUrl(const QString &url);

    // 检查是否有更新
    void checkForUpdates();

    // 下载更新包
    void downloadUpdate();

    // 安装更新
    void installUpdate();

signals:
    // 检查更新结果
    void updateCheckFinished(bool hasUpdate, const QString &latestVersion, const QString &releaseNotes);
    // 下载进度
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    // 下载完成
    void downloadFinished(const QString &filePath);
    // 错误信息
    void errorOccurred(const QString &errorMessage);

private slots:
    void onVersionCheckFinished();
    void onUpdateDownloaded();

private:
    QNetworkAccessManager *m_networkManager;
    QVersionNumber m_currentVersion;
    QUrl m_versionInfoUrl;
    QUrl m_updatePackageUrl;
    QString m_latestVersion;
    QString m_releaseNotes;
    QString m_downloadedFilePath;
};

#endif // UPDATEMANAGER_H
