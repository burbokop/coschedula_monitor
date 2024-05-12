#pragma once

#include <QObject>
#include <QQmlListProperty>
#include <QtQmlIntegration>
#include "logitem.h"
#include <coschedula/scheduler.h>

class Task : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("created by Monitor only")

    Q_PROPERTY(bool suspended MEMBER m_suspended NOTIFY suspendedChanged)
    Q_PROPERTY(bool finished MEMBER m_finished NOTIFY finishedChanged)
    Q_PROPERTY(QString location READ location CONSTANT)
    Q_PROPERTY(quint64 startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(quint64 endTime READ endTime WRITE setEndTime NOTIFY endTimeChanged)
    Q_PROPERTY(quint64 workTime READ workTime WRITE setWorkTime NOTIFY workTimeChanged)
    Q_PROPERTY(QQmlListProperty<LogItem> log READ log NOTIFY logChanged)

public:
    Task(const coschedula::scheduler::task_info &data,
         TimePoint startTime,
         QObject *parent = nullptr)
        : QObject(parent)
        , m_handle(data.h)
        , m_suspended(data.suspended)
        , m_location(data.loc)
        , m_dep(data.dep)
        , m_log({new LogItem(LogItem::State::Started, startTime, this)})
    {}

    void markFinished()
    {
        if (m_finished)
            return;
        m_finished = true;
        emit finishedChanged();
    }
    QString location() const { return m_location.function_name(); }
    std::coroutine_handle<> handle() const { return m_handle; };
    QQmlListProperty<LogItem> log() const;

    const QList<LogItem *> &logList() const { return m_log; };

    void setSuspended(bool suspended)
    {
        if (m_suspended == suspended)
            return;
        m_suspended = suspended;
        emit suspendedChanged();
    }

    void addLog(LogItem::State state, TimePoint time)
    {
        if (!m_log.isEmpty()) {
            m_log.back()->setEndTime(time);

            if (m_log.back()->state() == LogItem::State::Resumed) {
                const auto duration = m_log.back()->endTimeNs() - m_log.back()->startTimeNs();
                setWorkTime(workTime() + duration);
            }
        }
        m_log.push_back(new LogItem(state, time, this));
        emit logChanged();
        setStartTime(m_log.isEmpty() ? 0 : m_log.front()->startTimeNs());
        setEndTime(m_log.isEmpty() ? 0 : m_log.back()->endTimeNs());
    }

    quint64 startTime() const;
    void setStartTime(quint64 newStartTime);

    quint64 endTime() const;
    void setEndTime(quint64 newEndTime);

    quint64 workTime() const;
    void setWorkTime(quint64 newWorkTime);

signals:
    void suspendedChanged();
    void finishedChanged();
    void logChanged();
    void startTimeChanged();
    void endTimeChanged();
    void workTimeChanged();

private:
private:
    std::coroutine_handle<> m_handle;
    bool m_suspended;
    coschedula::source_location m_location;
    std::optional<std::coroutine_handle<>> m_dep;
    bool m_finished = false;
    QList<LogItem *> m_log;
    quint64 m_startTime = 0;
    quint64 m_endTime = 0;
    quint64 m_workTime = 0;
};

class Monitor : public QObject
{
    friend TimePoint;
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("interface")

    Q_PROPERTY(QQmlListProperty<Task> tasks READ tasks NOTIFY tasksChanged)
    Q_PROPERTY(quint64 totalEndTime READ totalEndTime NOTIFY totalEndTimeChanged)
public:
    Monitor(QObject *parent = nullptr);
    QQmlListProperty<Task> tasks() const;

    quint64 totalEndTime() const { return m_totalEndTime ? m_totalEndTime->ns() : 0; }

    Q_INVOKABLE QPointF scaleAndTrans(qreal currentTrans,
                                      qreal currentScale,
                                      qreal scaleDivision,
                                      qreal wheelPos) const;

signals:
    void tasksChanged();
    void totalEndTimeChanged();

protected:
    template<typename C>
    void addTask(const coschedula::scheduler::task_info &data, std::chrono::time_point<C> timePoint)
    {
        if (!m_startNsTimePoint) {
            m_startNsTimePoint = TimePoint::nsSinceEpoch(timePoint);
        }

        m_tasks.push_back(new Task(data, TimePoint(this, timePoint), this));
        setTotalEndTime(TimePoint(this, timePoint));
        emit tasksChanged();
    }

    template<typename F>
    void updateTask(std::coroutine_handle<> h, F &&f)
    {
        const auto it = std::find_if(m_tasks.begin(), m_tasks.end(), [&h](const Task *t) {
            return t->handle() == h;
        });
        if (it != m_tasks.end()) {
            f(*it);
            setTotalEndTime((*it)->logList().back()->endTime());
        }
    }

private:
    void setTotalEndTime(TimePoint time)
    {
        if (m_totalEndTime == time)
            return;

        m_totalEndTime = time;
        emit totalEndTimeChanged();
    }

private:
    QList<Task *> m_tasks;
    std::optional<std::uint64_t> m_startNsTimePoint;
    std::optional<TimePoint> m_totalEndTime;
};
Q_DECLARE_INTERFACE(Monitor, "appcoschedula_monitor.Monitor")

template<std::derived_from<coschedula::scheduler> T>
class MonitorImpl : public Monitor, public coschedula::scheduler::subscriber
{
    //QML_ELEMENT
    using Clock = std::chrono::high_resolution_clock;

public:
    MonitorImpl(QObject *parent = nullptr)
        : Monitor(parent)
    {
        coschedula::scheduler::instance<T>.install_subscriber(*this);

        //QTimer *timer = new QTimer(this);
        //connect(timer, &QTimer::timeout, this, [this] {
        //    const auto tasks = coschedula::scheduler::instance<T>.tasks();
        //    QList<TaskInfo> ti;
        //    for (const auto &t : tasks) {
        //        ti.push_back(t);
        //    }
        //    //qDebug() << "tasks:" << ti.size();
        //    m_tasks = ti;
        //    emit tasksChanged();
        //});
        //timer->start(1000 / 60);
    }

    // subscriber interface
public:
    void task_started(const coschedula::scheduler::task_info &info) override
    {
        addTask(info, Clock::now());
    }

    void task_finished(const coschedula::scheduler::task_info &info) override
    {
        updateTask(info.h, [time = TimePoint(this, Clock::now())](Task *task) {
            task->markFinished();
            task->addLog(LogItem::State::Finished, time);
        });
    }

    void task_suspended(const coschedula::scheduler::task_info &info) override
    {
        updateTask(info.h, [time = TimePoint(this, Clock::now())](Task *task) {
            task->setSuspended(true);
            task->addLog(LogItem::State::Suspended, time);
        });
    }

    void task_resumed(const coschedula::scheduler::task_info &info) override
    {
        updateTask(info.h, [time = TimePoint(this, Clock::now())](Task *task) {
            task->setSuspended(false);
            task->addLog(LogItem::State::Resumed, time);
        });
    }
};
