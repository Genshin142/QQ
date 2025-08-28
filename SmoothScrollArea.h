#ifndef SMOOTHSCROLLAREA_H
#define SMOOTHSCROLLAREA_H

#include <QScrollArea>
#include <QPropertyAnimation>
#include <QWheelEvent>
#include <QScrollBar>

class SmoothScrollArea : public QScrollArea
{
    Q_OBJECT
    Q_PROPERTY(int verticalValue READ verticalValue WRITE setVerticalValue)
public:
    explicit SmoothScrollArea(QWidget *parent = nullptr);

    int verticalValue() const;
    void setVerticalValue(int value);
    void scrollToBottom();

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    QPropertyAnimation *m_animation;
    int m_targetValue;
};

#endif // SMOOTHSCROLLAREA_H
