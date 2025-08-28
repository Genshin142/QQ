#include "logoutworker.h"
#include <QDateTime>   // 日期时间处理类
#include <QFile>       // 文件操作类
#include <QTextStream> // 文本流操作类
#include <QDebug>      // 调试输出类
#include <QNetworkReply> // 网络响应类
#include <QJsonObject>   // JSON对象类
#include <QJsonDocument> // JSON文档类
#include <QDir>
#include <QCoreApplication>
// 日志工具函数（内部使用）：记录登出过程中的关键信息
static void writeLog(const QString& message, const QString& type = "INFO") {
    // 构建根目录下的"日志"文件夹路径
    QString logDirPath = QCoreApplication::applicationDirPath() + "/日志";
    // 检查并创建"日志"文件夹（不存在则创建）
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        if (!logDir.mkpath(".")) { // 创建文件夹（包括必要的父目录）
            qWarning() << "无法创建日志文件夹：" << logDirPath;
            return; // 创建失败则直接返回，避免后续错误
        }
    }

    // 获取当前日期并格式化为yyyyMMdd形式
    QString dateStr = QDateTime::currentDateTime().toString("yyyyMMdd");
    // 构建带日期的日志文件名，存储到日志文件夹下
    QString fileName = logDirPath + QString("/登出子线程日志_%1.log").arg(dateStr);

    // 1. 输出到控制台，便于开发调试
    qDebug() << QString("[%1] %2: %3")
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))  // 时间戳
                    .arg(type)  // 日志类型（INFO/ERROR）
                    .arg(message);  // 日志内容

    // 2. 写入日志文件，用于问题追踪
    QFile file(fileName);
    // 以追加模式打开文件（不存在则创建）
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);  // 创建文本流
        // 写入格式化的日志内容
        out << QString("[%1] %2: %3\n")
                   .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                   .arg(type)
                   .arg(message);
        file.close();  // 关闭文件
    } else {
        qWarning() << "无法打开日志文件：" << fileName << "，错误：" << file.errorString();
    }
}



// 构造函数：初始化登出工作对象
// 参数：userId-用户ID，parent-父对象
LogoutWorker::LogoutWorker(const QString& phone, QObject* parent)
    : QObject(parent)
    , m_userPhone(phone)  // 保存用户ID
    , m_manager(new QNetworkAccessManager(this))  // 创建网络访问管理器
{
    // 记录创建登出工作对象的日志
    writeLog(QString("创建登出工作对象，用户ID: %1").arg(m_userPhone));
    // 排队调用登出方法（确保在工作线程中执行）
    writeLog(QString("准备发起登出请求，用户ID: %1").arg(m_userPhone));
    QMetaObject::invokeMethod(this, "doLogout", Qt::QueuedConnection);
}

LogoutWorker::~LogoutWorker()
{
    writeLog("LogoutWorker 对象销毁");
}

// 执行登出：构建请求并发送到服务器
void LogoutWorker::doLogout()
{
    // 记录登出请求发起日志
    writeLog(QString("发起登出请求，用户ID: %1，请求地址: http://101.37.68.162:8080/api/logout")
                 .arg(m_userPhone));

    // 1. 设置请求URL
    QUrl url("http://101.37.68.162:8080/api/logout");
    QNetworkRequest request(url);
    // 设置请求头：指定内容类型为JSON
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    writeLog("已设置请求头为application/json");

    // 2. 构建JSON请求数据
    QJsonObject json;
    json["phone"] = m_userPhone;    // 用户ID
    json["online"] = 0;           // 设置在线状态为0（离线）
    QByteArray data = QJsonDocument(json).toJson();  // 转换为JSON字节数组
    writeLog("已构建登出请求JSON数据");

    // 3. 记录发送的数据
    writeLog(QString("发送登出数据: %1").arg(QString(data)));

    // 4. 连接网络响应信号与处理槽函数
    connect(m_manager, &QNetworkAccessManager::finished, this, &LogoutWorker::onReplyFinished);
    writeLog("已建立网络响应信号连接，等待服务器响应");
    // 发送POST请求
    m_manager->post(request, data);
}

// 处理服务器响应：解析返回结果并发出相应信号
void LogoutWorker::onReplyFinished(QNetworkReply* reply)
{
    writeLog("开始处理服务器登出响应");
    // 1. 处理网络错误
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = "网络错误: " + reply->errorString();
        writeLog(errorMsg, "ERROR");  // 记录错误日志
        emit errorOccurred(errorMsg);  // 发出错误信号
        reply->deleteLater();  // 释放响应对象
        return;
    }

    // 2. 读取并记录响应数据
    QByteArray responseData = reply->readAll();
    writeLog(QString("收到响应数据: %1").arg(QString(responseData)));

    // 3. 解析JSON响应
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull()) {  // JSON解析失败
        QString errorMsg = "解析错误: 响应数据格式不正确";
        writeLog(errorMsg, "ERROR");
        emit errorOccurred(errorMsg);
        reply->deleteLater();
        return;
    }
    writeLog("JSON响应解析成功");

    // 4. 处理登出结果
    QJsonObject response = doc.object();
    if (response["success"].toBool()) {  // 登出成功
        QString successMsg = "登出成功";
        // 记录成功日志
        writeLog(QString("登出成功，用户ID: %1").arg(m_userPhone));
        // 发出登出成功信号
        emit logoutResult(true, successMsg);
    } else {  // 登出失败
        QString errorMsg = response["error"].toString();  // 获取错误信息
        // 记录失败日志
        writeLog(QString("登出失败，用户ID: %1，原因: %2").arg(m_userPhone).arg(errorMsg), "ERROR");
        // 发出登出失败信号
        emit logoutResult(false, errorMsg);
    }

    // 5. 清理资源并发出完成信号
    reply->deleteLater();  // 释放响应对象
    writeLog("登出处理流程完成，释放资源");
    emit finished();       // 通知登出过程完成
}
