#include "event_pool.hpp"
#include "event_loop.hpp"
#include <iterator>
#include <iostream>
#include <thread>
#include <barrier>

EventPool::EventPool(int time, int stationSize, int trainsSize, State* state) {
    this->pool = std::multiset<Event>();
    this->globalTime = 0;
    this->tOffset = 0;
    this->maxTime = time;
    this->target = {-1};
    this->targetInfo = {Event(0,-1,-1,false)};
    this->maxSize = stationSize;
    this->trainsSize = trainsSize;
    this->state = state;
    this->file = std::ofstream("out/time.csv");
    this->file << "t" << std::endl;
    std::cout << state->trains.size() << state->stations.size() << std::endl;
    for (int i = 0; i < trainsSize; ++i) {
        this->dispatch(Event(10*i, state->getTrain(i).getStartIndex(), i, false)); //set off the trains, choo choo
    }
}


int EventPool::progressTime(std::barrier<>& syncPoint) {
    //print the pool for debugging
    
    int target = -1;
    int trainIndex = -1;
    int t = 0; int nextT = 0; bool multi = false; bool propogate = false; bool entryExit = false;
    Event eCopy = Event(0,-1,-1,false); //dummy initialised
    int tOffsetNow = this->globalTime;
    {std::lock_guard<std::mutex> lock(this->poolMutex);
        //std::cout << "Current Event Pool: " << std::endl;
        //for (const auto& e : this->pool) {
        //    std::cout << "Event Time: " << e.getTime() << " Target: " << e.getTarget() << "Train: " << e.getTrainIndex() << "Global time: " << this->globalTime << std::endl;
        //}

        //std::cout << "BAR" << std::endl;
    
        if (this->pool.empty()) {
            std::cout << "No events to progress time to." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            this->target = {-1}; //reset to no event target
            this->targetInfo = {Event(0,-1,-1,false)};
            return -1; //no events to progress time to
        }
        auto it = this->pool.begin();
        eCopy = (*it);
        t = (*it).getTime();
        nextT = (*std::next(it,1)).getTime();
        target = (*it).getTarget();
        trainIndex = (*it).getTrainIndex();
        entryExit = (*it).getEntryExit();
        this->pool.erase(it);
    }   
    if(nextT == t) {
        this->progressTime(syncPoint);
        multi = true;
    }
    else {
        this->tOffset = this->globalTime;
    }
    if(!multi) {
        this->globalTime += t - tOffsetNow;
        this->file << std::to_string(this->globalTime) << std::endl;
        std::cout << "TIME PROGRESS " << this->globalTime << std::endl;
    }
    //send request to target at event time


    if (this->globalTime > this->maxTime) {
        //end simulation
        std::cout << "Max simulation time reached." << std::endl;
        this->target = {}; //reset to no event target
        this->file.close();
        return -2;
    }

    //set up next target
    Train thisTrain = this->state->getTrain(trainIndex);
    int direction = thisTrain.getDirection();
    
    //send request for station to process event (actioned now)
    this->sendRequest(target, eCopy);

    //start setting up next event to be dispatched
    
    int nextTarget = target;
    int timeToTarget = 40;

    if(entryExit) { //if we have just left a station
    
        if(target > 0 && target < this->maxSize - 1) {
            if(direction == 1) {
                nextTarget = target + 1;
                timeToTarget = this->state->stations[target].getTimeForward();
            }
            else {
                nextTarget = target -1;
                timeToTarget = this->state->stations[target].getTimeBackward();
            }
        }
        else if(target == this->maxSize - 1) {
            timeToTarget = this->state->stations[target].getTimeBackward();
            if(direction == 1) {
                this->state->changeTrainDirection(trainIndex);
                timeToTarget = this->state->stations[target].getTimeForward();
            }
            nextTarget = target - 1;
        }
        else if(target == 0) {
            timeToTarget = this->state->stations[target].getTimeForward();
            if(direction == -1) {
                this->state->changeTrainDirection(trainIndex);
                timeToTarget = this->state->stations[target].getTimeBackward();
            }
            nextTarget = target + 1;
        }
        if(target != -1) {
            this->dispatch(Event(timeToTarget+this->globalTime, nextTarget, trainIndex, false)); //entry request at next station
        }
    }
    else {
        this->dispatch(Event(this->state->stations[target].getTimeToLeave()+this->globalTime, target, trainIndex, true)); //leave request 
    }
    return 0;
}

int EventPool::getGlobalTime() {
    std::lock_guard<std::mutex> lock(this->poolMutex);
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

int EventPool::sendRequest(int target, Event e) {
    //send request to station at target index
    this->target.push_back(target);
    this->targetInfo.push_back(e);
    return 0;
}

std::vector<int> EventPool::getTargets() {
    return this->target;
}

std::vector<Event> EventPool::getTargetInfo() {
    return this->targetInfo;
}

EventPool::~EventPool() {
    this->pool = std::multiset<Event>();
    //deallocate and clean up threads
}