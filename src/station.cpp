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
        if(this->request2 == true) { //if request 2 arrives first, we don't unlock the barrier
                this->request2 = false;
                continue;
        }
        if (this->request == true) { //check for request 2 as well
            //check busy, set event
            this->request = false; //if request 1 arrives first, this is fine, we continue the loop here. If request 2 arrives first no action is taken
            std::thread t = std::thread([](std::barrier<>& sync){sync.arrive_and_wait();}, std::ref(syncPoint));
            t.detach();
        }
    }
}

void Station::stop() {
    this->running = false;
}

void Station::receiveLeaveRequest() {
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