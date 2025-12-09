#ifndef STATION_HPP
#define STATION_HPP

#include <string>
#include <mutex>
#include <barrier>
#include <vector>
#include <fstream>
#include "event.hpp"

#include <boost/json.hpp>
#include <boost/numeric/odeint.hpp>
#include "ode/ode_solver.hpp"
#include "station_ode.hpp"

class State;

namespace odeint = boost::numeric::odeint;

class Station {
  private:
  std::mutex stationMutex;

  std::string name;
  std::ofstream file;
  int capacity;
  int timeToLeaveRequest;

  ODE_Solver::Solver<odeint::runge_kutta_dopri5<ODE_Solver::Vector>, StationSystem,
                     ODE_Solver::Vector>
    station_ode_system;

  std::atomic<bool> running;
  std::vector<bool> platformStatus;  // only edited internally
  std::vector<int> leaveRequests;    // train index/id
  std::vector<double> leaveRequestTimestamps;
  std::vector<int> entryRequests;  // train index/id
  std::vector<double> entryRequestTimestamps;
  std::vector<int> platforms;  // platforms with integer for direction, setup defined

  public:
  Station(std::string name, std::vector<int> platforms, boost::json::object json_definition);
  Station(Station&& other) noexcept
      : name(other.getName()), station_ode_system(other.station_ode_system) {
    this->running.store(true);
  };
  Station(const Station&) = delete;

  Station& operator=(const Station&) = delete;

  void listen(std::barrier<>& syncPoint, State& state);
  void stop();
  Event receiveEntryRequest(Event e, State* state);
  Event receiveLeaveRequest(Event e, State* state);
  std::string getName() { return this->name; }

  void exportCurState();
  void finishExport();
  ~Station();
};

#endif