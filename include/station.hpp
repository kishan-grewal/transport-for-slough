#ifndef STATION_HPP
#define STATION_HPP

#include <atomic>
#include <memory>
#include <string>

class Station {
private:
    std::string name;
    int capacity;
    int population;
    int timeToLeaveRequest;

    bool busy;
    bool request;
    bool running = true;

public:
    Station(std::string name);
    void listen();
    void stop();
    void sendLeaveRequest();
    void receiveLeaveRequest();
    ~Station();
};

#endif