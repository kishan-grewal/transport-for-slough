#ifndef STATION_HPP
#define STATION_HPP

#include <atomic>
#include <memory>
#include <string>
#include <mutex>
#include <barrier>
#include <vector>

class Station {
private:
    std::mutex stationMutex;

    std::string name;
    int capacity;
    int population;
    int timeToLeaveRequest;

    std::atomic<bool> running;
    std::vector<bool> platformStatus; //only edited internally
    std::vector<int> platforms; //platforms with integer for direction, setup defined

public:
    Station(std::string name, std::vector<int> platforms);
    Station(Station&& other) noexcept : name(other.getName()) {this->running.store(true);};
    Station(const Station&) = delete;
    Station& operator=(const Station&) = delete;
    void listen(std::barrier<>& syncPoint);
    void stop();
    void receiveEntryRequest(int direction);
    void receiveLeaveRequest(int direction);
    std::string getName() {return this->name;}
    ~Station();
};

#endif