#ifndef EVENT_HPP
#define EVENT_HPP

class Event {
private:
    int time; //in seconds <timestep> to request trigger
    int targetIndex; //index to target of request being dispatched
    bool increaseOthers; //if this event is to cause a delay, set this flag to true
    //add time attribute to all elements of the queue and remove this event immediately

public:
    Event(int t, int index);
    int addTime(int t); //modify the time to event
    int getTime();

    bool operator<(Event& rhs); //comparison operator defined for multiset sorting
    ~Event();
};

#endif