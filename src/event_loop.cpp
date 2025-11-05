#include "event_loop.hpp"
#include <mutex>
#include "station.hpp"
#include <iostream>

EventLoop::EventLoop(int time) {
    this->state.stations = {Station("Stratford"),Station("West Ham"),Station("Canning Town"),Station("North Greenwich"),Station("Canary Wharf"), Station("Canada Water"),
                      Station("Bermondsey"),Station("London Bridge"),Station("Southwark"),Station("Waterloo"),};

    this->state.trains = std::vector{Train(100, 0, 10, 1), Train(100, 1, 10, 1), Train(100, 2, 10, 1)};
    this->events = std::make_shared<EventPool>(time, this->state.stations.size(), this->state.trains.size(), &(this->state)); //pass this for access to global state
}

void EventLoop::start() {
    for(int i = 0; i < this->state.stations.size(); ++i) {
        this->stationThreads.push_back(std::thread(&Station::listen, &(this->state.stations[i])));
    } //undefined ref to station listen here

    std::this_thread::sleep_for(std::chrono::seconds(1));
    this->runThread = std::make_shared<std::thread>(&EventLoop::run, this);
}

void EventLoop::run() {
    while (this->running.load()) {
        int res = this->events.load()->progressTime();
        if(res == -2) {
            this->running.store(false);
            break;
        }
        int target = this->events.load()->getTarget();
        std::cout << "Dispatching event to station index: " << target << std::endl;
        if(target >= this->state.stations.size()) {
            target = 0;
        }
        if (target != -1) {
            this->state.stations[target].receiveLeaveRequest();
        }
    }
}

EventLoop::~EventLoop() {
    for (int i = 0; i < stationThreads.size(); ++i) {
        this->state.stations[i].stop(); //change to other function
        //that will notify the threads to terminate their loops
    }

    for (int i = 0; i < stationThreads.size(); ++i) {
        if (this->stationThreads[i].joinable()) {
            this->stationThreads[i].join(); 
        }
    }
}