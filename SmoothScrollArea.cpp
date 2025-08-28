#include "SmoothScrollArea.h"
#include <QScrollBar>

SmoothScrollArea::SmoothScrollArea(QWidget *parent)
    : QScrollArea(parent), m_targetValue(0)
{
    m_animation = new QPropertyAnimation(this, "verticalValue");
    m_animation->setDuration(400); // 400ms 滚动时长
    m_animation->setEasingCurve(QEasingCurve::OutCubic); // 减速曲线更顺滑
}

int SmoothScrollArea::verticalValue() const
{
    return verticalScrollBar()->value();
}

void SmoothScrollArea::setVerticalValue(int value)
{
    verticalScrollBar()->setValue(value);
}

void SmoothScrollArea::scrollToBottom()
{
    int max = verticalScrollBar()->maximum();
    m_targetValue = max;
    m_animation->stop();
    m_animation->setStartValue(verticalScrollBar()->value());
    m_animation->setEndValue(max);
    m_animation->start();
}

void SmoothScrollArea::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    m_targetValue = verticalScrollBar()->value() - delta;

    // 限制范围
    m_targetValue = qMax(0, qMin(m_targetValue, verticalScrollBar()->maximum()));

    m_animation->stop();
    m_animation->setStartValue(verticalScrollBar()->value());
    m_animation->setEndValue(m_targetValue);
    m_animation->start();
}
