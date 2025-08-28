// enrollworker.h
#ifndef ENROLLWORKER_H
#define ENROLLWORKER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QString>

class EnrollWorker : public QObject
{
    Q_OBJECT

public:
    explicit EnrollWorker(const QString& phoneNumber, const QString& name, const QString& password, QObject* parent = nullptr);
public slots:
    void doEnroll();

signals:
    void enrollResult(bool success, const QString& message);
    void errorOccurred(const QString& errorMsg);
    void finished();

private:
    QString m_phoneNumber;
    QString m_name;
    QString m_password;
};

#endif // ENROLLWORKER_H
