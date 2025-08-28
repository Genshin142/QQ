// loginworker.h
#ifndef LOGINWORKER_H
#define LOGINWORKER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>

class LoginWorker : public QObject
{
    Q_OBJECT

public:
    explicit LoginWorker(const QString& account, const QString& password, QObject* parent = nullptr);
    ~LoginWorker();
public slots:
    void doLogin();

signals:
    void loginResult(bool success, const QString& message, const QString& userName = "", const QString& userId = "", const QString& userPhone = "");
    void errorOccurred(const QString& errorMsg);
    void finished();

private:
    QString m_account;
    QString m_password;
};

#endif // LOGINWORKER_H
