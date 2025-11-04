#include "event_pool.hpp"
#include <iterator>
#include <iostream>
#include <thread>

EventPool::EventPool(int time) {
    this->pool = std::multiset<Event>();
    this->globalTime = 0;
    this->tOffset = 0;
    this->maxTime = time;
    this->target = -1;

    this->dispatch(Event(5, 1));
    this->dispatch(Event(10, 0));
    //this->dispatch(Event(15, 2));
}


int EventPool::progressTime() {
    //print the pool for debugging
    {
    std::lock_guard<std::mutex> lock(this->poolMutex);
    std::cout << "Current Event Pool: " << std::endl;
    for (const auto& e : this->pool) {
        std::cout << "Event Time: " << e.getTime() << " Target: " << e.getTarget() << "Global time: " << this->globalTime << std::endl;
    }
    
    if (this->pool.empty()) {
        std::cout << "No events to progress time to." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        this->target = -1; //reset to no event target
        return -1; //no events to progress time to
    }
    auto it = this->pool.begin();
    int t = (*it).getTime();
    int tAgg = t - this->tOffset;
    int disturbanceLocation = (*it).getTarget();
    if((*it).propogate()) { //propogation events happen instantly increasing ttl
        t += 0;
        //find all events that share target
        //erase and re insert for modification of times
    }
    else {
        this->tOffset = this->globalTime;
    }
    this->globalTime += tAgg;
    //send request to target at event time
    
    int target = (*it).getTarget();
    this->pool.erase(it);
    this->sendRequest(target);
    }
    if(target < 5 && target != -1) {
        this->dispatch(Event(40+this->globalTime, target+1));
    }

    if(target == 5) {
        std::cout << "End of line reached, no further events dispatched." << std::endl;
    }

    return 0;
}

int EventPool::getGlobalTime() {
    return this->globalTime;
} 

int EventPool::getPoolEmpty() {
    std::lock_guard<std::mutex> lock(this->poolMutex);
    return this->pool.empty();
}

int EventPool::dispatch(Event e) {
    std::lock_guard<std::mutex> lock(this->poolMutex);
    this->pool.insert(e); 
    return 0; //lock goes out of scope at end of function
}

int EventPool::sendRequest(int target) {
    //send request to station at target index
    this->target = target;
    return 0;
}

int EventPool::getTarget() {
    return this->target;
} 

EventPool::~EventPool() {
    this->pool = std::multiset<Event>();
    //deallocate and clean up threads
}