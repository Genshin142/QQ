#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMouseEvent> // 添加头文件
#include <QPropertyAnimation>
#include <QColor>
#include <QMutex>
#include <QNetworkReply>
#include <QStandardItem>
#include <QTreeWidgetItem>
#include "localmessagemanager.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE
// 日志类型枚举
enum LogType {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};
class Widget : public QWidget
{
    Q_OBJECT
    // 用于背景渐变动画的属性
    Q_PROPERTY(QColor color1 READ color1 WRITE setColor1)
    Q_PROPERTY(QColor color2 READ color2 WRITE setColor2)

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void init_tabwidget();//初始化tabwidget
    void init_tabWidget_contact_person();
    void init_groupbox();
    void init_label_title();
    void init_label_avatar();
    void init_pushButton_contact_person();
    void init_pushButton_chat();
    void init_background_color();          // 初始化背景渐变动画
    void init_treeWidget_friend();
    void init_treeWidget_group();
    void initListWidgetRecord();
    void initFriendList();
    void initTreeWidgetFriend();
    void init_label_name();
    void onTreeWidgetFriendItemDoubleClicked(QTreeWidgetItem *item, int column);
    // 背景渐变相关的颜色访问接口
    QColor color1() const { return m_color1; }
    QColor color2() const { return m_color2; }
    void setColor1(const QColor &color) { m_color1 = color; update(); }
    void setColor2(const QColor &color) { m_color2 = color; update(); }
    QString readAvatarPath();      // 读取头像路径
    void saveAvatarPath(const QString &path);  // 保存头像路径
    bool eventFilter(QObject *watched, QEvent *event) override; // 事件过滤器
    void displayMessages(const QJsonArray& messages);
    void onSendMessageFinished();
    QString getDisplayTime(const QDateTime &time);
    void uploadAvatarToServer(const QString &filePath);
    void onLoginSuccess();
    void fetchAvatarFromServer(const QString &userId);
    void fetchAllAvatars();


protected:
    // 重写事件
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_pushButton_chat_clicked();

    void on_pushButton_contact_person_clicked();



    // 新增：获取所有用户信息响应处理
    void onGetAllUsersFinished(QNetworkReply* reply);
    // 新增：获取所有用户信息
    void getAllUsers();
    // 新增：ListView好友项点击处理
    void onFriendListViewItemClicked(const QModelIndex &index);
    // 新增：发送消息按钮点击
    void on_pushButton_send_clicked();
    void fetchNewMessages();
    void onMessagesFetched(QNetworkReply* reply);
    // 日志工具函数
    static void writeLog(const QString& message, LogType type = LOG_INFO);
    QString getUserNameById(const QString& userId);


private:
    Ui::Widget *ui;
    QColor m_color1;               // 背景渐变起始色
    QColor m_color2;               // 背景渐变结束色
    QPropertyAnimation *m_anim1;   // 颜色动画1
    QPropertyAnimation *m_anim2;   // 颜色动画2
    // 日志文件相关（静态成员需类外初始化）
    static QMutex logMutex;
    static QString logFileName;
    QNetworkAccessManager* m_networkManager;
    QStandardItemModel* m_chatListModel;
    // 新增：当前聊天对象信息
    QString m_currentFriendId;    // 当前聊天好友ID
    QString m_currentFriendName;  // 当前聊天好友名称
    // 改为维护每个好友的最后消息ID映射
    QMap<QString, QString> m_lastMessageIds;  // key:好友ID, value:最后消息ID
    // 新增：用户ID到用户名的映射
    QMap<QString, QString> m_userIdToName;  // key: userId, value: userName
    QSet<QString> m_localMessageIds;  // 存储本地已显示的消息ID
    bool hasNewMessages ; // 标志是否有新消息
    QMap<QString, QDateTime> m_lastMessageTime;  // 记录每个好友的最后消息时间
    float m_scrollAccumulator = 0.0f; // 滚动累积器





};
#endif // WIDGET_H
