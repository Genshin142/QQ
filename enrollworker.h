// enrollworker.h
#ifndef ENROLLWORKER_H
#define ENROLLWORKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QWidget>

class EnrollWorker : public QObject                     // 注册功能工作类(用于子线程中执行注册逻辑)
{
    Q_OBJECT

public:
    explicit EnrollWorker(const QString& phoneNumber, const QString& name, const QString& password, QObject* parent = nullptr);  // 构造函数(初始化用户注册信息)
    void doEnroll();                                     // 执行注册逻辑(构建请求并发送到服务器)
    void onReplyFinished(QNetworkReply* reply);          // 处理网络响应(解析服务器返回结果)

signals:
    void enrollResult(bool success, const QString& message);  // 注册结果信号(通知主线程成功/失败)
    void errorOccurred(const QString& errorMsg);          // 错误信号(通知主线程网络或解析错误)
    void finished();                                      // 完成信号(通知线程清理资源)

private:
    QString m_phoneNumber;                                // 存储用户手机号(注册账号)
    QString m_name;                                       // 存储用户昵称
    QString m_password;                                   // 存储用户密码
    QNetworkAccessManager* m_manager;                     // 网络请求管理器(处理HTTP通信)
};

#endif // ENROLLWORKER_H
