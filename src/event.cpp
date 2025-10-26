#include "event.hpp"
#include <exception>

Event::Event(int t, int index) {
    this->time = t;
    this->targetIndex = index;
}

int Event::addTime(int t) {
    this->time += t;
    if(this->time <= 0) {
        throw std::logic_error("TimeError, event should have executed");
    }
}

int Event::getTime() {
    return this->time;
}

bool Event::operator<(Event& rhs) {
    return this->time < rhs.getTime();
}