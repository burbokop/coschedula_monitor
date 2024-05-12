#include "monitor.h"
#include "matrix.h"

#include <QPainter>

Monitor::Monitor(QObject *parent)
    : QObject(parent)
{}

QQmlListProperty<Task> Monitor::tasks() const
{
    QQmlListProperty<Task> prop(const_cast<Monitor *>(this), &const_cast<Monitor *>(this)->m_tasks);
    prop.append = nullptr;
    prop.clear = nullptr;
    prop.replace = nullptr;
    prop.removeLast = nullptr;
    return prop;
}

namespace {

using Float = qreal;

Matrix<Float> filterAcceptsScale(Matrix<Float> v)
{
    return Matrix<Float>::scale(v.scaleX(), v.scaleY());
}

Matrix<Float> filterAcceptsTranslation(Matrix<Float> v)
{
    return Matrix<Float>::translate(v.translation().x(), v.translation().y());
}

template<typename T>
bool updateDifferent(T &out, const T &val)
{
    if (out == val)
        return false;

    out = val;
    return true;
}

bool concatScaleCentered(Matrix<Float> &scaleOutput,
                         Matrix<Float> &translationOutput,
                         Float scaleDivision,
                         const QPointF &center)
{
    using M = Matrix<Float>;
    const auto scaleDivisionMatrix = M::scale(scaleDivision);
    const auto translation = M::translate(center);
    const auto invTranslation = ~translation;
    assert(invTranslation);
    const auto output = translation * scaleDivisionMatrix * (*invTranslation) * translationOutput
                        * scaleOutput;

    {
        const auto newScaleOutput = filterAcceptsScale(output);
        const auto newTranslationOutput = filterAcceptsTranslation(output);

        const auto sok = updateDifferent(scaleOutput, newScaleOutput);
        const auto tok = updateDifferent(translationOutput, newTranslationOutput);
        return sok || tok;
    }
}
} // namespace

QPointF Monitor::scaleAndTrans(qreal currentTrans,
                               qreal currentScale,
                               qreal scaleDivision,
                               qreal wheelPos) const
{
    auto t = Matrix<Float>::translate(currentTrans, currentTrans);
    auto s = Matrix<Float>::scale(currentScale, currentScale);
    const auto center = QPointF(wheelPos, wheelPos);

    concatScaleCentered(s, t, scaleDivision, center);
    return {t.translation().x(), s.scaleX()};
}

QQmlListProperty<LogItem> Task::log() const
{
    QQmlListProperty<LogItem> prop(const_cast<Task *>(this), &const_cast<Task *>(this)->m_log);
    prop.append = nullptr;
    prop.clear = nullptr;
    prop.replace = nullptr;
    prop.removeLast = nullptr;
    return prop;
}

quint64 Task::startTime() const
{
    return m_startTime;
}

void Task::setStartTime(quint64 newStartTime)
{
    if (m_startTime == newStartTime)
        return;
    m_startTime = newStartTime;
    emit startTimeChanged();
}

quint64 Task::endTime() const
{
    return m_endTime;
}

void Task::setEndTime(quint64 newEndTime)
{
    if (m_endTime == newEndTime)
        return;
    m_endTime = newEndTime;
    emit endTimeChanged();
}

quint64 Task::workTime() const
{
    return m_workTime;
}

void Task::setWorkTime(quint64 newWorkTime)
{
    if (m_workTime == newWorkTime)
        return;
    m_workTime = newWorkTime;
    emit workTimeChanged();
}
