#include "logitem.h"
#include "monitor.h"

namespace {

template<typename T>
T safeMinus(T a, T b)
{
    Q_ASSERT(a >= b);
    return a - b;
}
} // namespace

TimePoint::TimePoint(Monitor *monitor, uint64_t timestamp)
    : m_ns(safeMinus(timestamp, monitor->m_startNsTimePoint.value()))
{}

LogItem::LogItem(State state, TimePoint startTime, Task *parent)
    : QObject(parent)
    , m_state(state)
    , m_startTime(startTime)
    , m_endTime(startTime)
{}
