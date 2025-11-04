#include "event.hpp"
#include <exception>
#include <stdexcept>

Event::Event(int t, int index) {
    this->time = t;
    this->targetIndex = index;
}

int Event::getTarget() const {
    return this->targetIndex;
}

int Event::getTime() const{
    return this->time;
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