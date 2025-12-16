#include "station.hpp"
#include "state.hpp"
#include <string>
#include <iostream>
#include <thread>
#include <barrier>
#include <mutex>

Station::Station(std::string name, std::vector<int> platforms, boost::json::object json_definition,
                 std::string split_ratios_path, std::string flows_path)
    : station_ode_system(odeint::runge_kutta_dopri5<ODE_Solver::Vector>(),
                         StationSystem(json_definition, split_ratios_path, flows_path, 1,
                                       "out/stations/" + name + " flows.csv"),
                         StationFileObserver("out/stations/" + name + ".csv")) {
  this->name = name;
  this->running.store(true);

  this->platforms = platforms;
  // if (this->platforms.size() != station_ode_system.system.platform_count())
  //   throw std::runtime_error("Mismatched station definition and platform vector");

  for (int i = 0; i < this->platforms.size(); ++i) {
    this->platformStatus.push_back(false);
    this->leaveRequests.push_back(-1);
    this->leaveRequestTimestamps.push_back(-1);
    this->entryRequests.push_back(-1);  // consider using ints instead of fixed size requests
    this->entryRequestTimestamps.push_back(-1);
  }

  std::string fname = "out/" + name + ".csv";
  this->file = std::ofstream(fname);
  std::string cols = "population";
  for (int i = 0; i < this->platforms.size(); ++i) {
    cols += ",platform " + std::to_string(i);
  }
  this->file << cols << std::endl;
}

void Station::listen(std::barrier<>& syncPoint, State& state) {
  // listen for events from event pool
  int popEnter = 0;
  int popLeave = 0;
  while (this->running.load()) {
    // for each step in time we recalculate populations
    {
      std::lock_guard<std::mutex> lock(this->stationMutex);
      for (int platform_i = 0; platform_i < this->platforms.size(); ++platform_i) {
        if (this->entryRequests[platform_i] > -1) {  // Arrival
          popEnter += 10000;
          int train_id = this->entryRequests[platform_i];
          double timestamp = this->entryRequestTimestamps[platform_i];
          this->entryRequests[platform_i] = -1;  // replace with train id

          this->station_ode_system.SolveToTime(timestamp,
                                               this->station_ode_system.system.InputDriver());
          try {
            Train train = state.getTrain(train_id);
            this->station_ode_system.LastState() +=
              this->station_ode_system.system.PlatformUpdateVector(train.disembark(this->name),
                                                                   platform_i);

            int platform_segment =
              this->station_ode_system.system.QueryPlatformDepartingIndex(platform_i);
            int person_count = int(this->station_ode_system.LastState()[platform_segment]);

            person_count = train.try_embark(person_count, this->name);
            this->station_ode_system.LastState() +=
              this->station_ode_system.system.PlatformUpdateVector(-person_count, platform_i);
          } catch (std::runtime_error err) {
            std::cout << err.what() << std::endl;
            std::cout << "WARN - invalid station platform id " << platform_i
                      << " for solver. Station " << this->name << " has "
                      << this->station_ode_system.system.platform_count() << " platforms."
                      << std::endl;
          }
        }
        if (this->leaveRequests[platform_i] > -1) {  // Departure
          this->leaveRequests[platform_i] = -1;
        }
      }
    }

    syncPoint.arrive_and_wait();

    // std::cout << this->name << " population: " << this->population << std::endl;
    this->exportCurState();

    syncPoint.arrive_and_wait();
  }
}

void Station::stop() { this->running.store(false); }

Event Station::receiveEntryRequest(Event e, State* state) {
  std::lock_guard<std::mutex> lock(this->stationMutex);

  // std::cout << "Entry request received at " << this->name << std::endl;
  std::this_thread::sleep_for(std::chrono::microseconds(1));
  for (int i = 0; i < this->platforms.size(); ++i) {
    if ((this->platforms[i] == state->getTrain(e.getTrainIndex()).getDirection() ||
         !(this->platforms[i])) &&
        this->platformStatus[i] == false) {
      this->platformStatus[i] = true;  // make busy
      this->entryRequests[i] = e.getTrainIndex();
      this->entryRequestTimestamps[i] = e.getTime();
      return Event(0, -1, -1, false);
    }
  }
  // add time to current event and return to be re added to event pool
  e.propogate(40);
  return e;
}

Event Station::receiveLeaveRequest(Event e, State* state) {
  std::lock_guard<std::mutex> lock(this->stationMutex);

  // std::cout << "Leave request received at " << this->name << std::endl;
  std::this_thread::sleep_for(std::chrono::microseconds(1));
  for (int i = 0; i < this->platforms.size(); ++i) {
    if ((this->platforms[i] == state->getTrain(e.getTrainIndex()).getDirection() ||
         !(this->platforms[i])) &&
        this->platformStatus[i] == true) {
      this->platformStatus[i] = false;  // make free
      this->leaveRequests[i] = e.getTrainIndex();
      this->leaveRequestTimestamps[i] = e.getTime();
      return Event(0, -1, -1, false);  // empty event signals no new event to add
    }
  }
  // add time to current event and return to be re added to event pool

  e.propogate(40);
  return e;
}

void Station::exportCurState() {
  std::string status = "";
  for (int i = 0; i < this->platformStatus.size(); ++i) {
    status += "," + std::to_string(this->platformStatus[i]);
  }
  // this->file << this->population << status << std::endl;
}

void Station::finishExport() { this->file.close(); }

Station::~Station() { this->name = ""; }