#include "event_pool.hpp"
#include <iterator>

EventPool::EventPool() {
    this->pool = std::multiset<Event>();
    this->globalTime = 0;
}

int EventPool::dispatchNextEvent() {
    //send request to target thread
    return 0;
}

int EventPool::progressTime(int t) {
    for(auto i : this->pool) {
        i.addTime(-t);
    }
    this->globalTime += t;

    this->dispatchNextEvent();
}

EventPool::~EventPool() {
    this->pool = std::multiset<Event>();
    //deallocate and clean up threads
}