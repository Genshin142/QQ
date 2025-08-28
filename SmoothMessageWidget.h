#ifndef SMOOTHMESSAGEWIDGET_H
#define SMOOTHMESSAGEWIDGET_H

#include <QWidget>
#include <QString>
#include <QDateTime>
#include <QLabel>
#include <QPixmap>

class SmoothMessageWidget : public QWidget
{
    Q_OBJECT
public:
    enum Side { Left, Right };
    explicit SmoothMessageWidget(const QString &userId, const QString &userName, const QString &content, 
                               const QString &timeStr, Side side, bool isGroup, bool isImage, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUi();
    QPixmap getRoundAvatar(const QString &userId);
    QString formatTime(const QString &timeStr);

    QString m_userId;
    QString m_userName;
    QString m_content;
    QString m_time;
    Side m_side;
    bool m_isGroup;
    bool m_isImage;

    QLabel *m_avatarLabel;
    QLabel *m_nameLabel;
    QWidget *m_bubbleWidget; // This will be the custom painted part
};

// Sub-class for the actual bubble to handle custom painting
class MessageBubblePart : public QWidget
{
    Q_OBJECT
public:
    MessageBubblePart(const QString &text, bool isRight, bool isImage, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_text;
    bool m_isRight;
    bool m_isImage;
    QPixmap m_image;
};

#endif // SMOOTHMESSAGEWIDGET_H
