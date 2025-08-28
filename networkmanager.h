#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QByteArray>
#include <QDataStream>

class NetworkManager : public QObject {
    Q_OBJECT
public:
    static NetworkManager& instance() {
        static NetworkManager inst;
        return inst;
    }

    void init(const QString& host = "127.0.0.1", quint16 port = 8080);
public slots:
    void sendPacket(const QJsonObject& json);
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void loginResponse(bool success, const QString& message, const QJsonObject& data);
    void registerResponse(bool success, const QString& message);
    void logoutResponse(bool success, const QString& message);
    void checkPhoneResponse(bool exists, const QString& userId);
    void allUsersResponse(const QJsonArray& users);
    void allAvatarsResponse(const QJsonArray& avatars);
    void avatarResponse(const QString& userId, bool success, const QByteArray& imgData);
    void historyMessagesResponse(const QJsonArray& messages);
    void newMessage(const QJsonObject& msg);
    void friendRequest(const QJsonObject& data);
    void pongReceived();
    void errorOccurred(const QString& error);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void sendHeartbeat();

private:
    NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();
    
    QTcpSocket* m_socket;
    QByteArray m_buffer;
    QTimer* m_heartbeatTimer;
    QString m_host;
    quint16 m_port;
};

#endif // NETWORKMANAGER_H
