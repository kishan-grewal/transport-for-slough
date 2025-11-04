#include "event_loop.hpp"
#include <mutex>
#include "station.hpp"
#include <iostream>

EventLoop::EventLoop(int time) {
    this->events = std::make_shared<EventPool>(time);
    this->stations = {Station("Stratford"),Station("West Ham")};
}

void EventLoop::start() {
    for(int i = 0; i < this->stations.size(); ++i) {
        this->stationThreads.push_back(std::thread(&Station::listen, &(this->stations[i])));
    } //undefined ref to station listen here

    std::this_thread::sleep_for(std::chrono::seconds(1));
    this->runThread = std::make_shared<std::thread>(&EventLoop::run, this);
}

void EventLoop::run() {
    while (this->running.load()) {
        std::cout << "Event Loop progressing time..." << std::endl;
        this->events.load()->progressTime();
        std::cout << "Event Loop dispatched event." << std::endl;
        int target = this->events.load()->getTarget();
        std::cout << "Dispatching event to station index: " << target << std::endl;
        if (target != -1) {
            this->stations[target].receiveLeaveRequest();
        }
    }
}

EventLoop::~EventLoop() {
    for (int i = 0; i < stationThreads.size(); ++i) {
        this->stations[i].stop(); //change to other function
        //that will notify the threads to terminate their loops
    }

    for (int i = 0; i < stationThreads.size(); ++i) {
        if (this->stationThreads[i].joinable()) {
            this->stationThreads[i].join(); 
        }
    }
}