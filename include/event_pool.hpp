#include "event.hpp"
#include "station.hpp"

#ifndef EVENT_POOL_HPP
#define EVENT_POOL_HPP

#include <set>

class EventPool {
private:
    std::multiset<Event> pool;
    int globalTime = 0;
    //send a request to the station you would like for event to occur on
    int sendRequest(std::shared_ptr<Station> target);
    int progressTime(int t);

public:
    EventPool();
    int dispatch(Event e);
    int getGlobalTime();
    ~EventPool();
};

#endif