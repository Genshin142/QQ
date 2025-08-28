// logoutworker.h
#ifndef LOGOUTWORKER_H
#define LOGOUTWORKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>

class LogoutWorker : public QObject                     // 登出功能工作类(在子线程中处理登出逻辑)
{
    Q_OBJECT

public:
    explicit LogoutWorker(const QString& phone, QObject* parent = nullptr);  // 构造函数(初始化用户ID)
    ~LogoutWorker();
    void doLogout();                                    // 执行登出逻辑(构建请求并发送到服务器)
    void onReplyFinished(QNetworkReply* reply);         // 处理网络响应(解析服务器返回的登出结果)

signals:
    void logoutResult(bool success, const QString& message);  // 登出结果信号(通知主线程成功/失败)
    void errorOccurred(const QString& errorMsg);          // 错误信号(通知主线程网络或解析错误)
    void finished();                                      // 完成信号(登出逻辑结束后触发,用于线程清理)

private:
    QString m_userPhone;                                    // 存储用户ID
    QNetworkAccessManager* m_manager;                    // 网络请求管理器(处理与服务器的HTTP通信)
};

#endif // LOGOUTWORKER_H
