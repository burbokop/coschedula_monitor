#pragma once

#include <QObject>
#include <QtQmlIntegration>

class Monitor;
class Task;

class TimePoint
{
public:
    template<typename C>
    TimePoint(Monitor *monitor, std::chrono::time_point<C> timePoint)
        : TimePoint(monitor, nsSinceEpoch(timePoint))
    {}

    template<typename C>
    static std::uint64_t nsSinceEpoch(std::chrono::time_point<C> timePoint)
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(timePoint.time_since_epoch())
            .count();
    }

    std::uint64_t ns() const { return m_ns; };

    std::strong_ordering operator<=>(const TimePoint &) const = default;

private:
    TimePoint(Monitor *monitor, std::uint64_t timestamp);

private:
    std::uint64_t m_ns;
};

class LogItem : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("created by Task only")
public:
    enum class State { Started, Suspended, Resumed, Finished };

private:
    Q_ENUM(State)
    Q_PROPERTY(State state MEMBER m_state CONSTANT)
    Q_PROPERTY(quint64 startTime READ startTimeNs NOTIFY startTimeChanged)
    Q_PROPERTY(quint64 endTime READ endTimeNs NOTIFY endTimeChanged)
public:
    explicit LogItem(State state, TimePoint startTime, Task *parent);

    State state() const { return m_state; }

    TimePoint startTime() const { return m_startTime; }
    TimePoint endTime() const { return m_endTime; }

    quint64 startTimeNs() const { return m_startTime.ns(); }
    quint64 endTimeNs() const { return m_endTime.ns(); }

    void setEndTime(TimePoint time)
    {
        if (m_endTime == time)
            return;

        m_endTime = time;
        emit endTimeChanged();
    }

signals:
    void startTimeChanged();
    void endTimeChanged();

private:
    State m_state;
    TimePoint m_startTime;
    TimePoint m_endTime;
};
