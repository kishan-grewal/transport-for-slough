#include "event_loop.hpp"
#include "station.hpp"
#include <iostream>

//! EventLoop constructor
/*!
Create an EventLoop object
\param time int maximum simulation time
\param initState State the initial program state of stations and trains
\param stationSize int the number of stations in the network being simulated

Creates a shared pointer to the event pool object. Creates a barrier with n_stations + 1
*/
EventLoop::EventLoop(int time, State initState, int stationSize)
    : state(std::move(initState)), barrier(stationSize + 1) {
  int s = this->state.stations.size();
  int s2 = this->state.trains.size();
  this->events = std::make_shared<EventPool>(time, s, s2, &(this->state));
}

//! Start the EventLoop
/*!
Creates new thread for each station's listen function, passing the barrier
Creates a new thread for the eventloop run function
*/
void EventLoop::start() {
  for (int i = 0; i < this->state.stations.size(); ++i) {
    this->stationThreads.push_back(std::thread(&Station::listen, &(this->state.stations[i]),
                                               std::ref(this->barrier), std::ref(this->state)));
  }  // undefined ref to station listen here
  this->runThread = std::make_shared<std::thread>(&EventLoop::run, this);
}

//! Main event loop function that repeats
/*!
While running, progress the event pool by consuming events at the next timestamp
If the simulation reaches max time, res = -2 and the loop will break
Otherwise load the current event pool (atomically)
Fetch the stations which will receive event requests
Fetch the corresponding event requests
Depending on the event type, send a leave/entry request to corresponding station
If the request is denied, a propogated request is returned, adding a fixed time on top of the time the request was made
Add this to the event pool and remove the predicted event that wasn't taken
Otherwise event is processed by the station
Empty the targets for this timestamp and wait at double barrier before repeating the loop
*/
void EventLoop::run() {
  Event propogation = {Event(0, -1, -1, false)};
  while (this->running.load()) {
    int res = this->events.load()->progressTime(std::ref(this->barrier));
    if (res == -2) {
      this->running.store(false);
      for (int i = 0; i < this->state.stations.size(); ++i) {
        this->state.stations[i].finishExport();
      }
      this->waitBarrier();
      this->waitBarrier();
      break;
    }
    std::shared_ptr<EventPool> cur = this->events.load();
    std::vector<int> targets =
      cur->getTargets();  // turn into vector for multiple events dispatched simultaneously
    std::vector<Event> targetInfo = cur->getTargetInfo();
    if (targets.size() == 0) {
      std::thread t = std::thread(&EventLoop::waitBarrier, this);
      t.detach();
    }
    else if (targets.size() == 1 && targets[0] < 0) {
      // prevent hang by releasing barrier when no events
      std::thread t = std::thread(&EventLoop::waitBarrier, this);
      t.detach();  // maybe remove?
    }
    else {
      for (int i{}; i < targets.size(); ++i) {
        if (targets[i] >= this->state.stations.size() && targets[i] != -1) {
          targets[i] = 0;
        }
        if (targets[i] != -1) {
          if (targetInfo[i].getEntryExit()) {  // if leaving train
            propogation =
              this->state.stations[targets[i]].receiveLeaveRequest(targetInfo[i], &(this->state));
          }
          else {
            propogation =
              this->state.stations[targets[i]].receiveEntryRequest(targetInfo[i], &(this->state));
          }
          if (propogation.getTrainIndex() != -1) {
            std::cout << "propogation" << std::endl;
            this->events.load()->dispatch(propogation);
            this->events.load()->eraseFirstNot(propogation); //delete the next event that was pre-emptively dispatched
          }
        }
      }
    }
    this->events.load()->emptyTargets();
    this->waitBarrier();  // synchronise detecting of events

    this->waitBarrier();  // synchronise state calculations

    // continue loop to repeat for next timestep
  }
}

//! Helper to reach barrier early
/*!
Wait at barrier
*/
void EventLoop::waitBarrier() { this->barrier.arrive_and_wait(); }

//! Event loop destructor, thread cleanup
/*!
Stop and join all threads
*/
EventLoop::~EventLoop() {
  for (int i = 0; i < stationThreads.size(); ++i) {
    this->state.stations[i].stop();  // change to other function
    // that will notify the threads to terminate their loops
  }

  for (int i = 0; i < stationThreads.size(); ++i) {
    if (this->stationThreads[i].joinable()) {
      this->stationThreads[i].join();
    }
  }

  this->runThread->join();
}