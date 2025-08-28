// UserManager.h
#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QString>

class UserManager {
public:
    // 获取单例实例
    static UserManager& getInstance() {
        static UserManager instance;
        return instance;
    }

    // 禁止拷贝构造和赋值操作
    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    // 设置用户ID
    void setUserId(const QString& userId) {
        m_userId = userId;
    }

    // 获取用户ID
    QString getUserId() const {
        return m_userId;
    }

    // 设置手机号
    void setPhoneNumber(const QString& phone) {
        m_phoneNumber = phone;
    }

    // 获取手机号
    QString getPhoneNumber() const {
        return m_phoneNumber;
    }

    // 设置用户名
    void setName(const QString& name) {
        m_userName = name;
    }

    // 获取用户名
    QString getName() const {
        return m_userName;
    }

    // 清除用户信息（登出时使用）
    void clearUserInfo() {
        m_userId.clear();
        m_phoneNumber.clear();
        m_userName.clear(); // 同时清除用户名
    }

private:
    // 私有构造函数
    UserManager() = default;
    QString m_userId;      // 存储用户ID
    QString m_phoneNumber; // 存储手机号
    QString m_userName;    // 新增：存储用户名
};

#endif // USERMANAGER_H
