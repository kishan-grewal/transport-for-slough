#ifndef EVENT_HPP
#define EVENT_HPP

class Event {
private:
    int time; //in seconds <timestep> to request trigger
    int targetIndex; //index to target of request being dispatched
    int trainIndex;
    bool entryExit;

public:
    Event(int t, int index, int trainIndex, bool entryExit);
    int getTime() const;
    int getTarget() const;
    int getTrainIndex() const;
    bool getEntryExit() const;
    void propogate(int addition);
    ~Event();
};

bool operator<(Event lhs, Event rhs);

#endif