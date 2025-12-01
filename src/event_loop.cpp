#include "event_loop.hpp"
#include <mutex>
#include "station.hpp"
#include <iostream>

//next send train info to station

EventLoop::EventLoop(int time, State initState, int stationSize) : state(std::move(initState)), barrier(stationSize+1) {
    int s = this->state.stations.size();
    int s2 = this->state.trains.size();
    this->events = std::make_shared<EventPool>(time, s, s2, &(this->state));
}

void EventLoop::start() {
    for(int i = 0; i < this->state.stations.size(); ++i) {
        this->stationThreads.push_back(std::thread(&Station::listen, &(this->state.stations[i]), std::ref(this->barrier)));
    } //undefined ref to station listen here

    std::this_thread::sleep_for(std::chrono::seconds(1));
    this->runThread = std::make_shared<std::thread>(&EventLoop::run, this);
}

void EventLoop::run() {
    Event propogation = {Event(0,-1,-1,false)};
    while (this->running.load()) {
        int res = this->events.load()->progressTime(std::ref(this->barrier));
        if(res == -2) {
            this->running.store(false);
            break;
        }
        std::shared_ptr<EventPool> cur = this->events.load();
        std::vector<int> targets = cur->getTargets(); //turn into vector for multiple events dispatched simultaneously
        std::vector<Event> targetInfo = cur->getTargetInfo();
        if(targets.size() == 0) {
            std::thread t = std::thread(&EventLoop::waitBarrier, this);
            t.detach();
        }
        else if(targets.size() == 1 && targets[0] < 0) {
            //prevent hang by releasing barrier when no events
            std::thread t = std::thread(&EventLoop::waitBarrier, this);
            t.detach(); //maybe remove?
        }
        else {
            for(int i{}; i < targets.size(); ++i) {
                if(targets[i] >= this->state.stations.size() && targets[i] != -1) {
                    targets[i] = 0;
                }
                if (targets[i] != -1) {
                    if (targetInfo[i].getEntryExit()) { //if leaving train
                        propogation = this->state.stations[targets[i]].receiveLeaveRequest(targetInfo[i], &(this->state));
                    }
                    else {
                        propogation = this->state.stations[targets[i]].receiveEntryRequest(targetInfo[i], &(this->state));
                    }
                    if (propogation.getTrainIndex() != -1) {
                        std::cout << "propogation" << std::endl;
                        this->events.load()->dispatch(propogation);
                    }
                }
            }
        }
        this->events.load()->emptyTargets();   
        this->waitBarrier(); //synchronise detecting of events

        this->waitBarrier(); //synchronise state calculations

        //continue loop to repeat for next timestep
    }
}

void EventLoop::waitBarrier() {
    this->barrier.arrive_and_wait();
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

    this->runThread->join();
}