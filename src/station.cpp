#include "station.hpp"
#include <string>
#include <iostream>
#include <thread>
#include <barrier>
#include <mutex>

Station::Station(std::string name) {
    this->name = name;
    this->running.store(true);
}

void Station::listen(std::barrier<>& syncPoint) {
    //listen for events from event pool

    while (this->running.load()) {
        {std::lock_guard<std::mutex> lock(this->stationMutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if(this->request2 == true) { //
                    this->request2 = false;
            }
            else if (this->request == true) { //add logic, directionality not matched to specific request yet
                //check busy, set event
                this->request = false;
            }
        }
        syncPoint.arrive_and_wait();

        //more logic for action based on request status

        syncPoint.arrive_and_wait();
    }
}

void Station::stop() {
    this->running.store(false);
}

void Station::receiveLeaveRequest() {
    std::lock_guard<std::mutex> lock(this->stationMutex);
    //stations now only receive one request at a time, modify this
    if(this->request2 == false) {
        this->request = true;
    }
    if(this->request == true) {
        this->request2 = true;
    }
    std::cout << "Leave request received at " << this->name << std::endl;
    std::this_thread::sleep_for(std::chrono::microseconds(1));
}

Station::~Station() {
    this->name = "";
}