#include "event_loop.hpp"

EventLoop::EventLoop() {
    this->events = std::atomic(std::make_shared<EventPool>());
    this->stations = {Station("Stratford"),Station("West Ham")};
}

void EventLoop::start(std::vector<Station> stations) {
    for(int i = 0; i < stations.length(); ++i) {
        this->stationThreads.push_back(std::thread(stations[i].listen));
    }
}

EventLoop::~EventLoop() {
    for (int i = 0; i < stationThreads.length(); ++i) {
        this->stationThreads.join(); //change to other function
        //that will notify the threads to terminate their loops
    }
}