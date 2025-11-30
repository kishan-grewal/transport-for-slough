#include "event.hpp"
#include <exception>
#include <stdexcept>

Event::Event(int t, int index, int trainIndex, bool entryExit) {
    this->time = t;
    this->targetIndex = index;
    this->trainIndex = trainIndex;
    this->entryExit = entryExit;
}

int Event::getTarget() const {
    return this->targetIndex;
}

int Event::getTime() const{
    return this->time;
}

int Event::getTrainIndex() const {
    return this->trainIndex;
}

bool Event::getEntryExit() const {
    return this->entryExit;
}

bool Event::propogate() const {
    return this->propogateOthers;
}

Event::~Event() {
    this->targetIndex = 0;
    this->time = 0;
}

bool operator<(Event lhs, Event rhs) {
    return lhs.getTime() < rhs.getTime();
}