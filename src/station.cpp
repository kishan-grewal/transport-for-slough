#include "station.hpp"
#include <string>
#include <iostream>
#include <thread>

Station::Station(std::string name) {
    this->name = name;
}

void Station::listen() {
    //listen for events from event pool

    while (this->running) {
        std::cout << "Listening at " << this->name << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (this->request == true) {
            //check busy, set event
            std::cout << "Event Received at " << this->name << std::endl;

            this->request = false;
        }
    }
}

void Station::stop() {
    this->running = false;
}

void Station::receiveLeaveRequest() {
    this->request = true;
    std::cout << "Leave request received at " << this->name << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

Station::~Station() {
    this->name = "";
}