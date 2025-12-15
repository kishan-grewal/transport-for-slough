#include "event_pool.hpp"
#include "event_loop.hpp"
#include <iterator>
#include <iostream>
#include <thread>
#include <barrier>

//! Event pool constructor
/*!
Creates an empty event pool (containing an event with invalid target)
\param time int maximum simulation time
\param stationSize int number of stations to simulate
\param trainsSize int number of trains to simulate
\param state State* pointer to the global state object

Internally uses a multiset of Event objects, which are sorted by time on insertion, nearest time first. Global time set to zero.

The event system uses indices for accessing stations and trains in the state, so the order of the items in the state *must* be preserved.
The number of stations and trains passed must be the number in the state or trains will be missed/out of bounds errors will occur.

The logging file for the timestamps is also created in this constructor.

Initial trains are set off here as well.
*/
EventPool::EventPool(int time, int stationSize, int trainsSize, State* state) {
  this->pool = std::multiset<Event>();
  this->globalTime = 0;
  this->tOffset = 0;
  this->maxTime = time;
  this->target = {-1};
  this->targetInfo = {Event(0, -1, -1, false)};
  this->maxSize = stationSize;
  this->trainsSize = trainsSize;
  this->state = state;
  this->file = std::ofstream("out/time.csv");
  this->file << "t" << std::endl;
  std::cout << "Trains: " << state->trains.size() << " Stations: " << state->stations.size()
            << std::endl;
  for (int i = 0; i < trainsSize; ++i) {
    this->dispatch(
      Event(10, state->getTrain(i).getStartIndex(), i, false));  // set off the trains, choo choo
  }
}

//! progress event pool function
/*!
progressTime consumes the event that is about to happen (and falls through multiple that have the same timestamp)
\param syncPoint std::barrier<>& reference to the barrier that synchronises the stations for lockstep execution

The consumed event is accessed and deleted inside the mutex lock scope, thus all necessary fields are copied into local variables.

Dispatches an invalid event if the pool is empty to keep the program running
Recursively calls multiple events that have the same timestamp, setting a flag multi to signal that the time shouldn't be updated until all of the concurrent events are consumed.

Closes the file and sends simulation time finished to the main program

Sends request to the target station of the event

Sets up next event to dispatch, does not listen whether request is accepted or rejected (BAD)
*/
int EventPool::progressTime(std::barrier<>& syncPoint) {
  // print the pool for debugging

  int target = -1;
  int trainIndex = -1;
  int t = 0;
  int nextT = 0;
  bool multi = false;
  bool propogate = false;
  bool entryExit = false;
  Event eCopy = Event(0, -1, -1, false);  // dummy initialised
  int tOffsetNow = this->globalTime;
  {
    std::lock_guard<std::mutex> lock(this->poolMutex);
    // std::cout << "Current Event Pool: " << std::endl;
    // for (const auto& e : this->pool) {
    //     std::cout << "Event Time: " << e.getTime() << " Target: " << e.getTarget() << "Train: "
    //     << e.getTrainIndex() << "Global time: " << this->globalTime << std::endl;
    // }

    // std::cout << "BAR" << std::endl;

    if (this->pool.empty()) {
      std::cout << "No events to progress time to." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
      this->target = {-1};  // reset to no event target
      this->targetInfo = {Event(0, -1, -1, false)};
      return -1;  // no events to progress time to
    }
    auto it = this->pool.begin();
    eCopy = (*it);
    t = (*it).getTime();
    nextT = (*std::next(it, 1)).getTime();
    target = (*it).getTarget();
    trainIndex = (*it).getTrainIndex();
    entryExit = (*it).getEntryExit();
    this->pool.erase(it);
  }
  if (nextT == t) {
    this->progressTime(syncPoint);
    multi = true;
  }
  else {
    this->tOffset = this->globalTime;
  }
  if (!multi) {
    this->globalTime += t - tOffsetNow;
    this->file << std::to_string(this->globalTime) << std::endl;
    std::cout << "TIME PROGRESS " << this->globalTime << std::endl;
  }
  // send request to target at event time

  if (this->globalTime > this->maxTime) {
    // end simulation
    std::cout << "Max simulation time reached." << std::endl;
    this->target = {};  // reset to no event target
    this->file.close();
    return -2;
  }

  // set up next target
  Train thisTrain = this->state->getTrain(trainIndex);
  int direction = thisTrain.getDirection();

  // send request for station to process event (actioned now)
  this->sendRequest(target, eCopy);

  // start setting up next event to be dispatched

  int nextTarget = target;

  if (entryExit) {  // if we have just left a station

    if (target > 0 && target < this->maxSize - 1) {
      if (direction == 1) {
        nextTarget = target + 1;
      }
      else {
        nextTarget = target - 1;
      }
    }
    else if (target == this->maxSize - 1) {
      if (direction == 1) {
        this->state->changeTrainDirection(trainIndex);
      }
      nextTarget = target - 1;
    }
    else if (target == 0) {
      if (direction == -1) {
        this->state->changeTrainDirection(trainIndex);
      }
      nextTarget = target + 1;
    }
    if (target != -1) {
      this->dispatch(Event(40 + this->globalTime, nextTarget, trainIndex,
                           false));  // entry request at next station
    }
  }
  else {
    this->dispatch(Event(40 + this->globalTime, target, trainIndex, true));  // leave request
  }
  return 0;
}

int EventPool::getGlobalTime() {
  std::lock_guard<std::mutex> lock(this->poolMutex);
  return this->globalTime;
}

int EventPool::getPoolEmpty() {
  std::lock_guard<std::mutex> lock(this->poolMutex);
  return this->pool.empty();
}

int EventPool::dispatch(Event e) {
  std::lock_guard<std::mutex> lock(this->poolMutex);
  this->pool.insert(e);
  return 0;  // lock goes out of scope at end of function
}

//! Erase function to get rid of conflicting predicted dispatched events
/*!
Function that erases other events the same train has dispatched that are now classed invalid

Probably should have a regular set now with different sorting criteria
*/
int EventPool::eraseFirstNot(Event e) {
  std::lock_guard<std::mutex> lock(this->poolMutex);
  auto it = this->pool.begin();
    
  while (it != pool.end()) {
    if (it->getTrainIndex() == e.getTrainIndex() && it->getTarget() != e.getTarget()) {
      it = pool.erase(it);
    }
    else {
      ++it;
    }
  }
}

int EventPool::sendRequest(int target, Event e) {
  // send request to station at target index
  this->target.push_back(target);
  this->targetInfo.push_back(e);
  return 0;
}

std::vector<int> EventPool::getTargets() { return this->target; }

std::vector<Event> EventPool::getTargetInfo() { return this->targetInfo; }

EventPool::~EventPool() {
  this->pool = std::multiset<Event>();
  // deallocate and clean up threads
}