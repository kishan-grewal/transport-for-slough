#include "station.hpp"
#include "state.hpp"
#include <string>
#include <iostream>
#include <thread>
#include <barrier>
#include <mutex>

Station::Station(std::string name, std::vector<int> platforms) {
    this->name = name;
    this->running.store(true);
    this->platforms = platforms;
    this->population = 100000;
    for(int i = 0; i < this->platforms.size(); ++i) {
        this->platformStatus.push_back(false);
        this->leaveRequests.push_back(-1);
        this->entryRequests.push_back(-1); //consider using ints instead of fixed size requests
    }
    std::cout << platforms[0] << std::endl;

    std::string fname = "out/" + name + ".csv";
    this->file = std::ofstream(fname);
    std::string cols = "population";
    for(int i = 0; i < this->platforms.size(); ++i) {
        cols += ",platform " + std::to_string(i);
    }
    this->file << cols << std::endl;
}

void Station::listen(std::barrier<>& syncPoint) {
    //listen for events from event pool
    int popEnter = 0;
    int popLeave = 0;
    while (this->running.load()) {
        //for each step in time we recalculate populations
        {std::lock_guard<std::mutex> lock(this->stationMutex);
            for (int i = 0; i < this->platforms.size(); ++i) {
                if (this->entryRequests[i] > -1) {
                    popEnter += 10000;
                    this->entryRequests[i] = -1; //replace with train id
                }
                if (this->leaveRequests[i] > -1) {
                    popLeave += 1000;
                    this->leaveRequests[i] = -1;
                }
            }
        }

        syncPoint.arrive_and_wait();

        //population calculations
        this->population += popEnter;
        this->population -= popLeave;
        popEnter = 0;
        popLeave = 0;

        std::cout << this->name << " population: " << this->population << std::endl;
        this->exportCurState();

        syncPoint.arrive_and_wait();
    }
}

void Station::stop() {
    this->running.store(false);
}

Event Station::receiveEntryRequest(Event e, State* state) {
    std::lock_guard<std::mutex> lock(this->stationMutex);

    std::cout << "Entry request received at " << this->name << std::endl;
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    for(int i = 0; i < this->platforms.size(); ++i) {
        if((this->platforms[i] == state->getTrain(e.getTrainIndex()).getDirection() || !(this->platforms[i])) && this->platformStatus[i] == false) {
            this->platformStatus[i] = true; //make busy
            this->entryRequests[i] = e.getTrainIndex();
            return Event(0,-1,-1,false);
        }
    }
    //add time to current event and return to be re added to event pool
    e.propogate(40);
    return e;
}

Event Station::receiveLeaveRequest(Event e, State* state) {
    std::lock_guard<std::mutex> lock(this->stationMutex);

    std::cout << "Leave request received at " << this->name << std::endl;
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    for(int i = 0; i < this->platforms.size(); ++i) {
        if((this->platforms[i] == state->getTrain(e.getTrainIndex()).getDirection() || !(this->platforms[i])) && this->platformStatus[i] == true) {
            this->platformStatus[i] = false; //make free
            this->leaveRequests[i] = e.getTrainIndex();
            return Event(0,-1,-1,false); //empty event signals no new event to add
        }
    }
    //add time to current event and return to be re added to event pool

    e.propogate(40);
    return e;
}

void Station::exportCurState() {
    std::string status = "";
    for(int i = 0; i < this->platformStatus.size(); ++i) {
        status += "," + std::to_string(this->platformStatus[i]);
    }
    this->file << this->population << status << std::endl;
}

void Station::finishExport() {
    this->file.close();
}

Station::~Station() {
    this->name = "";
}