#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include "station.hpp"
#include "event_pool.hpp"

#ifndef EVENT_LOOP_HPP
#define EVENT_LOOP_HPP

//spawn a thread for eventpool to run on
//spawn threads for the stations to run on
//move global time to event loop
//progress time and dispatch request to station thread
//station progresses state and dispatches new event to loop
//alternatively if failed request, modify time of the request that called to repeat later

class EventLoop {
private:
    std::atomic<std::shared_ptr<EventPool>> events;
    std::vector<std::thread> stationThreads{};
    std::vector<Station> stations; //maybe station, inherits from an agent class
    std::shared_ptr<std::thread> runThread;
    std::atomic<bool> running = true;
public:
    EventLoop(int time = 0);
    void start(); //spawn thread for EventPool operations
    void run();
    ~EventLoop();
};

#endif