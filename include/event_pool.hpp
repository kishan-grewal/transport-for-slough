#ifndef EVENT_POOL_HPP
#define EVENT_POOL_HPP
#include <mutex>
#include "state.hpp"
#include <set>
#include "event.hpp"

class EventPool {
private:
    std::multiset<Event> pool;
    std::mutex poolMutex;
    int globalTime = 0;
    int maxTime = 0;
    int tOffset = 0; //multiset is immutable
    int target = -1;
    int maxSize = 0;
    int trainsSize = 0;
    State* state;
    //rather than subtract the times from individual events
    //store the number to subtract from all elements and compute
    //on event fetch

    //send a request to the station you would like for event to occur on
    int sendRequest(int target);

public:
    EventPool(int time, int stationSize, int trainsSize, State* loop);
    int dispatch(Event e);
    int getGlobalTime();
    int getTarget();
    int progressTime();
    int getPoolEmpty();
    ~EventPool();
};

#endif