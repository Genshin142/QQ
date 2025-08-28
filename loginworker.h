// loginworker.h
#ifndef LOGINWORKER_H
#define LOGINWORKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QWidget>

class LoginWorker : public QObject                     // 登录功能工作类(在子线程中处理登录逻辑)
{
    Q_OBJECT

public:
    explicit LoginWorker(const QString& account, const QString& password, QObject* parent = nullptr);  // 构造函数(初始化登录账号和密码)
    ~LoginWorker();
    void doLogin();                                     // 执行登录逻辑(构建请求并发送到服务器)
    void onReplyFinished(QNetworkReply* reply);          // 处理网络响应(解析服务器返回的登录结果)


signals:
    void loginResult(bool success, const QString& message, const QString& userName = "",const QString& userId="",const QString& userPhone="");  // 登录结果信号(通知主线程成功/失败及用户名)
    void errorOccurred(const QString& errorMsg);          // 错误信号(通知主线程网络或解析错误)
    void finished();                                      // 完成信号(登录逻辑结束后触发,用于线程清理)

private:
    QString m_account;                                   // 存储登录账号(如手机号/用户名)
    QString m_password;                                  // 存储登录密码
    QNetworkAccessManager* m_manager;                    // 网络请求管理器(处理与服务器的HTTP通信)
};

#endif // LOGINWORKER_H
