#include "networkmanager.h"
#include <QDebug>
#include <QDateTime>
#include <QThread>
#include <QJsonArray> // Added QJsonArray include
#include <QJsonDocument> // Added QJsonDocument include
#include <QJsonObject> // Added QJsonObject include
#include <QDataStream> // Added QDataStream include

NetworkManager::NetworkManager(QObject* parent) : QObject(parent), m_socket(new QTcpSocket(this)) {
    connect(m_socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(m_socket, (void (QAbstractSocket::*)(QAbstractSocket::SocketError))&QAbstractSocket::errorOccurred, this, &NetworkManager::onError);

    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &NetworkManager::sendHeartbeat);
}

NetworkManager::~NetworkManager() {
    if (m_socket->isOpen()) {
        m_socket->close();
    }
}

void NetworkManager::init(const QString& host, quint16 port) {
    m_host = host;
    m_port = port;
    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        m_socket->connectToHost(m_host, m_port);
    }
}

bool NetworkManager::isConnected() const {
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::sendPacket(const QJsonObject& json) {
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, [this, json](){ this->sendPacket(json); }, Qt::QueuedConnection);
        return;
    }

    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "NetworkManager: Cannot send packet, socket not connected";
        return;
    }

    QByteArray jsonData = QJsonDocument(json).toJson(QJsonDocument::Compact);
    QByteArray packet;
    QDataStream ds(&packet, QIODevice::WriteOnly);
    ds << (quint32)jsonData.size();
    packet.append(jsonData);
    m_socket->write(packet);
}

void NetworkManager::onConnected() {
    qDebug() << "NetworkManager: Connected to server";
    m_heartbeatTimer->start(30000); // 30s heartbeat
    emit connected();
}

void NetworkManager::onDisconnected() {
    qDebug() << "NetworkManager: Disconnected from server. Reconnecting in 5s...";
    m_heartbeatTimer->stop();
    emit disconnected();
    
    // Auto reconnect
    QTimer::singleShot(5000, this, [this]() {
        if (m_socket->state() == QAbstractSocket::UnconnectedState) {
            m_socket->connectToHost(m_host, m_port);
        }
    });
}

void NetworkManager::onError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError); // Added Q_UNUSED
    qDebug() << "NetworkManager Socket Error:" << m_socket->errorString(); // Simplified debug output
    emit errorOccurred(m_socket->errorString()); // Simplified emit
}

void NetworkManager::onReadyRead() {
    m_buffer.append(m_socket->readAll());

    while (m_buffer.size() >= 4) {
        QDataStream ds(m_buffer);
        quint32 size;
        ds >> size;

        if (m_buffer.size() < 4 + size) break;

        QByteArray jsonData = m_buffer.mid(4, size);
        m_buffer.remove(0, 4 + size);

        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (doc.isNull() || !doc.isObject()) continue;

        QJsonObject json = doc.object();
        QString type = json["type"].toString();

        if (type == "login_res") {
            emit loginResponse(json["success"].toBool(), json["error"].toString(), json);
        } else if (type == "register_res") {
            emit registerResponse(json["success"].toBool(), json["error"].toString());
        } else if (type == "logout_res") {
            emit logoutResponse(json["success"].toBool(), json["error"].toString());
        } else if (type == "check_phone_res") {
            emit checkPhoneResponse(json["status"].toString() == "exist", json["id"].toString());
        } else if (type == "get_avatar_res") {
            QString userId = json["user_id"].toString();
            if (json["success"].toBool()) {
                QByteArray imgData = QByteArray::fromBase64(json["avatar_data"].toString().toUtf8());
                emit avatarResponse(userId, true, imgData);
            } else {
                emit avatarResponse(userId, false, QByteArray());
            }
        } else if (type == "get_all_users_res") {
            emit allUsersResponse(json["users"].toArray());
        } else if (type == "get_allAvatar_res") {
            emit allAvatarsResponse(json["avatars"].toArray());
        } else if (type == "get_messages_res") {
            emit historyMessagesResponse(json["messages"].toArray());
        } else if (type == "new_message") {
            emit newMessage(json);
        } else if (type == "new_friend_request_push") {
            emit friendRequest(json);
        } else if (type == "pong") {
            emit pongReceived();
        }
    }
}

void NetworkManager::sendHeartbeat() {
    QJsonObject json;
    json["type"] = "ping";
    sendPacket(json);
}
