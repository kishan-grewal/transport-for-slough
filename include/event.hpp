#ifndef EVENT_HPP
#define EVENT_HPP
//potentially not a templated thing?
class Event {
private:
    int time; //in seconds <timestep> to request trigger
    int targetIndex; //index to target of request being dispatched

public:
    Event(int t, int index);
    int addTime(int t); //modify the time to event

    bool operator<(Event& rhs); //comparison operator defined for multiset sorting
    ~Event();
};

#endif