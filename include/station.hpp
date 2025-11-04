#include <atomic>
#include <memory>

#ifndef STATION_HPP
#define STATION_HPP
#include <string>
#include "event_pool.hpp"

class Station {
private:
    std::string name;
    int capacity;
    int population;
    int timeToLeaveRequest;

    bool busy;
    bool request;
    bool running = true;

    std::shared_ptr<EventPool> p; //to dispatch events to

public:
    Station(std::string name);
    void listen();
    void stop();
    void sendLeaveRequest();
    void receiveLeaveRequest();
    ~Station();
};

#endif