#include "enrollworker.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <qdebug.h>
#include <QCoreApplication>
#include <QDir>
// 日志工具函数：输出到控制台并写入日志文件
// 参数：message-日志内容，type-日志类型（INFO/ERROR等）
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
    QString fileName = logDirPath + QString("/注册子线程日志_%1.log").arg(dateStr);

    // 格式化日志信息（包含时间戳）
    QString logMsg = QString("[%1] %2: %3")
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                         .arg(type)
                         .arg(message);

    // 输出到控制台
    qDebug() << logMsg;

    // 写入日志文件（追加模式）
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << logMsg << "\n";
        file.close();
    } else {
        qWarning() << "无法打开日志文件：" << fileName << "，错误：" << file.errorString();
    }
}



EnrollWorker::EnrollWorker(const QString& phone, const QString& name, const QString& password, QObject* parent)
    : QObject(parent)
    , m_phoneNumber(phone)
    , m_name(name)
    , m_password(password)
    , m_manager(new QNetworkAccessManager(this)) {
    writeLog(QString("创建注册工作对象，手机号: %1，昵称: %2").arg(phone).arg(name));
    writeLog("注册工作对象初始化完成，准备执行注册"); // 新增日志
}

// 执行注册逻辑：构建请求并发送
void EnrollWorker::doEnroll() {
    writeLog(QString("发起注册请求，目标地址: http://101.37.68.162:8080/api/register"));

    // 构建请求URL和头部信息
    QUrl url("http://101.37.68.162:8080/api/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    writeLog("已设置注册请求头为application/json"); // 新增日志

    // 构建JSON请求体
    QJsonObject json;
    json["phone"] = m_phoneNumber;
    json["name"] = m_name;
    json["password"] = m_password;
    QByteArray data = QJsonDocument(json).toJson();
    writeLog("已构建注册请求JSON数据"); // 新增日志

    // 日志中隐藏密码（用***代替）
    QString logData = QString(data).replace(m_password, "***");
    writeLog(QString("发送注册数据: %1").arg(logData));

    // 发送POST请求并关联响应处理函数
    connect(m_manager, &QNetworkAccessManager::finished, this, &EnrollWorker::onReplyFinished);
    writeLog("已建立注册响应信号连接，等待服务器响应"); // 新增日志
    m_manager->post(request, data);
}

// 处理注册响应
void EnrollWorker::onReplyFinished(QNetworkReply* reply) {
    writeLog("开始处理服务器注册响应"); // 新增日志
    // 检查网络错误
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = "网络错误: " + reply->errorString();
        writeLog(errorMsg, "ERROR");
        emit errorOccurred(errorMsg);
        reply->deleteLater();
        return;
    }

    // 读取并解析响应数据
    QByteArray responseData = reply->readAll();
    writeLog(QString("收到注册响应数据: %1").arg(QString(responseData)));

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull()) {
        QString errorMsg = "解析错误: 响应数据格式不正确";
        writeLog(errorMsg, "ERROR");
        emit errorOccurred(errorMsg);
        reply->deleteLater();
        return;
    }
    writeLog("注册响应JSON解析成功"); // 新增日志

    // 处理注册结果
    QJsonObject response = doc.object();
    if (response["success"].toBool()) {
        writeLog("注册成功");
        emit enrollResult(true, "账号注册成功！");
    } else {
        QString errorMsg = response["error"].toString();
        writeLog(QString("注册失败: %1").arg(errorMsg), "ERROR");
        emit enrollResult(false, errorMsg);
    }

    // 清理资源并通知完成
    reply->deleteLater();
    writeLog("注册处理流程完成，释放资源"); // 新增日志
    emit finished();
}
