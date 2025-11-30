#include "station.hpp"
#include <string>
#include <iostream>
#include <thread>
#include <barrier>
#include <mutex>

Station::Station(std::string name, std::vector<int> platforms) {
    this->name = name;
    this->running.store(true);
    this->platforms = platforms;
    for(int i = 0; i < this->platforms.size(); ++i) {
        this->platformStatus.emplace_back(false);
    }
    std::cout << platforms[0] << std::endl;
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

void Station::receiveEntryRequest(int direction) {
    std::lock_guard<std::mutex> lock(this->stationMutex);
    std::cout << "Entry request received at " << this->name << std::endl;
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    for(int i = 0; i < this->platforms.size(); ++i) {
        if(this->platforms[i] == direction && this->platformStatus[i] == false) {
            this->platformStatus[i] = true; //make busy
            return;
        }
    }
    //add time to current event
    return;
}

void Station::receiveLeaveRequest(int direction) {
    std::lock_guard<std::mutex> lock(this->stationMutex);

    std::cout << "Leave request received at " << this->name << std::endl;
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    for(int i = 0; i < this->platforms.size(); ++i) {
        if(this->platforms[i] == direction && this->platformStatus[i] == false) {
            this->platformStatus[i] = false; //make free
            return;
        }
    }
    //add time to current event
    return;
}

Station::~Station() {
    this->name = "";
}