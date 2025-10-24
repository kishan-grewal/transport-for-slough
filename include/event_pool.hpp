#include "event.hpp"
#include "station.hpp"

#ifndef EVENT_POOL_HPP
#define EVENT_POOL_HPP

#include <set>

class EventPool {
private:
    std::multiset<Event> pool;
    int globalTime = 0;

    int dispatchNextEvent();
    int progressTime(int t);

public:
    EventPool();
    int addEvent(Event e);
    int getGlobalTime();
    ~EventPool();
};

#endif