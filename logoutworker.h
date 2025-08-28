#ifndef LOGOUTWORKER_H
#define LOGOUTWORKER_H

#include <QObject>
#include <QString>

class LogoutWorker : public QObject
{
    Q_OBJECT

public:
    explicit LogoutWorker(const QString& phone, QObject* parent = nullptr);
    ~LogoutWorker();
    
public slots:
    void doLogout();

signals:
    void logoutResult(bool success, const QString& message);
    void errorOccurred(const QString& errorMsg);
    void finished();

private:
    QString m_userPhone;
};

#endif // LOGOUTWORKER_H
