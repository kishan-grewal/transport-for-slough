#include "station.hpp"
#include <string>
#include <iostream>
#include <thread>
#include <barrier>

Station::Station(std::string name) {
    this->name = name;
}

void Station::listen(std::barrier<>& syncPoint) {
    //listen for events from event pool

    while (this->running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (this->request == true) {
            //check busy, set event
            this->request = false;
            syncPoint.arrive_and_wait();
        }
    }
}

void Station::stop() {
    this->running = false;
}

void Station::receiveLeaveRequest() {
    this->request = true;
    std::cout << "Leave request received at " << this->name << std::endl;
    std::this_thread::sleep_for(std::chrono::microseconds(1));
}

Station::~Station() {
    this->name = "";
}