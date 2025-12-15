#include "event.hpp"
#include <exception>
#include <stdexcept>
#include <iostream>

//! Create Event object
/*!
Event object constructor
\param t int timestamp (wrt global time) for when event should be consumed
\param index int station for the event to be consumed
\param trainIndex int train that is involved in the event
\param entryExit bool True for exit, False for entry
*/
Event::Event(int t, int index, int trainIndex, bool entryExit) {
    this->time = t;
    this->targetIndex = index;
    this->trainIndex = trainIndex;
    this->entryExit = entryExit;
}

//! Get station of the event
int Event::getTarget() const {
    return this->targetIndex;
}

//! Get time of event
int Event::getTime() const{
    return this->time;
}

//! Get train of event
int Event::getTrainIndex() const {
    return this->trainIndex;
}

//! Get entry/exit status of event
bool Event::getEntryExit() const {
    return this->entryExit;
}

//! Propogate event
/*!
Add time to an event
\param addition int time to add to the event
*/
void Event::propogate(int addition) {
    std::cout << "propogating" << std::endl;
    this->time += addition; //does this lag from global time? maybe fetch too
}

//! Event destructor
/*!
Set event time, train and target to 0
*/
Event::~Event() {
    this->trainIndex = 0;
    this->targetIndex = 0;
    this->time = 0;
}

//! Comparator for ordering
/*!
Compare events by time
*/
bool operator<(Event lhs, Event rhs) {
    return lhs.getTime() < rhs.getTime();
}