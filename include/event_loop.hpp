#ifndef EVENT_LOOP_HPP
#define EVENT_LOOP_HPP
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <barrier>
#include "station.hpp"
#include "train.hpp"
#include "event_pool.hpp"
#include "state.hpp"

//spawn a thread for eventpool to run on
//spawn threads for the stations to run on
//move global time to event loop
//progress time and dispatch request to station thread
//station progresses state and dispatches new event to loop
//alternatively if failed request, modify time of the request that called to repeat later

//we want to just pass a bunch of trains and stations, and the event loop automatically manages them

class EventLoop {
private:
    std::atomic<std::shared_ptr<EventPool>> events;
    std::vector<std::thread> stationThreads{};
    std::shared_ptr<std::thread> runThread;
    std::barrier<> barrier;
    std::atomic<bool> running = true;
public:
    EventLoop(int time, State initState);
    State state; //for now global state is public
    void start(); //spawn thread for EventPool operations
    void run();
    void waitBarrier();
    ~EventLoop();
};

#endif