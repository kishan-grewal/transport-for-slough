#ifndef EVENT_POOL_HPP
#define EVENT_POOL_HPP
#include <mutex>
#include "state.hpp"
#include <set>
#include <fstream>
#include "event.hpp"

class EventPool {
private:
    std::multiset<Event> pool;
    std::mutex poolMutex;
    int globalTime = 0;
    int maxTime = 0;
    int tOffset = 0; //multiset is immutable
    std::vector<int> target = {};
    std::vector<Event> targetInfo = {};
    int maxSize = 0;
    int trainsSize = 0;
    State* state;
    //rather than subtract the times from individual events
    //store the number to subtract from all elements and compute
    //on event fetch

    std::ofstream file;

    //send a request to the station you would like for event to occur on
    int sendRequest(int target, Event e);

public:
    EventPool(int time, int stationSize, int trainsSize, State* loop);
    int dispatch(Event e);
    int eraseFirstNot(Event e);
    int getGlobalTime();
    std::vector<int> getTargets();
    std::vector<Event> getTargetInfo();
    void emptyTargets() {this->target = {-1}; this->targetInfo = {Event(0,-1,-1,false)};}
    int progressTime(std::barrier<>& syncPoint);
    int getPoolEmpty();
    ~EventPool();
};

#endif