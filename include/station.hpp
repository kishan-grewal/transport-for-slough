#ifndef STATION_HPP
#define STATION_HPP

#include <atomic>
#include <memory>
#include <string>
#include <mutex>
#include <barrier>

class Station {
private:
    std::mutex stationMutex;

    std::string name;
    int capacity;
    int population;
    int timeToLeaveRequest;

    bool busy;
    bool request = false;
    bool request2 = false;
    std::atomic<bool> running;

public:
    Station(std::string name);
    Station(Station&& other) noexcept : name(other.getName()) {this->running.store(true);};
    Station(const Station&) = delete;
    Station& operator=(const Station&) = delete;
    void listen(std::barrier<>& syncPoint);
    void stop();
    void sendLeaveRequest();
    void receiveLeaveRequest();
    std::string getName() {return this->name;}
    ~Station();
};

#endif