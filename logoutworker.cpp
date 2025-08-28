#include "logoutworker.h"
#include <QDateTime>   // 日期时间处理类
#include <QFile>       // 文件操作类
#include <QTextStream> // 文本流操作类
#include <QDebug>      // 调试输出类
#include <QNetworkReply> // 网络响应类
#include <QJsonObject>   // JSON对象类
#include <QJsonDocument> // JSON文档类
#include "networkmanager.h"
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



LogoutWorker::LogoutWorker(const QString& phone, QObject* parent)
    : QObject(parent)
    , m_userPhone(phone)
{
    writeLog(QString("创建 TCP 登出工作对象，手机号: %1").arg(m_userPhone));
}

LogoutWorker::~LogoutWorker()
{
}

void LogoutWorker::doLogout()
{
    writeLog(QString("发起 TCP 登出请求: %1").arg(m_userPhone));
    
    connect(&NetworkManager::instance(), &NetworkManager::logoutResponse, this, [this](bool success, const QString& message) {
        if (success) {
            emit logoutResult(true, "登出成功");
        } else {
            emit logoutResult(false, message);
        }
        emit finished();
    });

    QJsonObject json;
    json["type"] = "logout";
    json["phone"] = m_userPhone;
    NetworkManager::instance().sendPacket(json);
}


