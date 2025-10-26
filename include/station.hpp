#include <atomic>
#include <memory>

#include "event_pool.hpp"

#ifndef STATION_HPP
#define STATION_HPP

class Station {
private:
    int capacity;
    int population;
    int timeToLeaveRequest;

    bool busy;

    std::shared_ptr<EventPool> p; //to dispatch events to

public:
    Station();
    void listen();
    void sendLeaveRequest();
    ~Station();
};

#endif