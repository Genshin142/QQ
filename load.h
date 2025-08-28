#ifndef LOAD_H
#define LOAD_H

#include <QWidget>
#include <QColor>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QKeyEvent>
#include "updatemanager.h"
#include <QProgressDialog>

namespace Ui {
class load;
}

class load : public QWidget
{
    Q_OBJECT

    // 用于背景渐变动画的属性
    Q_PROPERTY(QColor color1 READ color1 WRITE setColor1)
    Q_PROPERTY(QColor color2 READ color2 WRITE setColor2)

signals:
    // 触发登录信号，将账号密码传递给工作线程处理
    void startLogin(const QString& account, const QString& password);

public:
    explicit load(QWidget *parent = nullptr);
    ~load();

    // 背景渐变相关的颜色访问接口
    QColor color1() const { return m_color1; }
    QColor color2() const { return m_color2; }
    void setColor1(const QColor &color) { m_color1 = color; update(); }
    void setColor2(const QColor &color) { m_color2 = color; update(); }

    // 界面初始化相关方法
    void init_background_color();          // 初始化背景渐变动画
    void init_label_avatar();              // 初始化头像标签
    void init_lineEdit_phone();            // 初始化手机号输入框
    void init_lineEdit_password();         // 初始化密码输入框
    void init_pushButton_load();           // 初始化登录按钮
    void init_pushButton_close();          // 初始化关闭按钮
    void init_pushButton_enroll();         // 初始化注册按钮
    void init_checkupdate();          // 初始化检查更新按钮
    void init_pushButton_forget_password();// 初始化忘记密码按钮
    void init_pushButton_clear();          // 初始化清除按钮
    void init_pushButton_choose();         // 初始化选择按钮
    void init_pushButton_clearPassword();  // 初始化密码清除按钮
     void init_checkBox_password();  // 初始化记住密码复选框
    // 新增：记住密码相关函数
    void saveLoginInfo(const QString& phone, const QString& password);
    void loadLoginInfo(QString& phone, QString& password);
    void clearSavedPassword();
    void checkPhoneNumber(const QString &phoneNumber);
    void downloadAndSetAvatar(const QString &user_id);
    void downloadDefaultAvatar();
    void initUpdateManager(); // 初始化更新管理器

    // 事件重写
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_pushButton_close_clicked();     // 关闭窗口按钮点击事件
    void checkInputValid();                 // 检查输入合法性
    void on_pushButton_enroll_clicked();    // 注册按钮点击事件
    void on_pushButton_load_clicked();      // 登录按钮点击事件
    void onLoginResult(bool success, const QString& message, const QString& userName = "",const QString& userId="",const QString& phone="");
    void onErrorOccurred(const QString& errorMsg);
    void on_pushButton_clear_clicked();     // 清除账号按钮点击事件
    void on_pushButton_clearPassword_clicked(); // 清除密码按钮点击事件
    void onUpdateCheckFinished(bool hasUpdate, const QString &latestVersion, const QString &releaseNotes);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(const QString &filePath);
    void onUpdateError(const QString &errorMessage);

private:
    Ui::load *ui;                  // UI界面对象指针
    QColor m_color1;               // 背景渐变起始色
    QColor m_color2;               // 背景渐变结束色
    QPropertyAnimation *m_anim1;   // 颜色动画1
    QPropertyAnimation *m_anim2;   // 颜色动画2
    QPoint mousePoint;             // 鼠标拖动位置记录
    bool mouse_press;              // 鼠标按下标志
    UpdateManager* m_updateManager;
    QProgressDialog* m_progressDialog ; // 下载进度对话框
    QString m_lastCheckedPhone;      // 记录上次检查过的手机号

};

#endif // LOAD_H
