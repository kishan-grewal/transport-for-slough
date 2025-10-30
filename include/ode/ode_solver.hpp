#ifndef ODE_SOLVER_HPP
#define ODE_SOLVER_HPP

#include <boost/json.hpp>
#include <boost/numeric/odeint.hpp>

#include <stdexcept>

#include "ode/ode_system.hpp"

namespace ODE_Solver {

template <class Stepper, class System, class State>
class Solver;

template <class Stepper, class System, class State>
class Solver {
  private:
  Stepper system_stepper;

  double last_update_time;
  State last_update_state;

  public:
  System system;

  Solver(Stepper stepper, System system, State state)
      : system_stepper(stepper), system(system), last_update_state(state) {
    this->last_update_time = 0;
  };
  // ~ODE_System();

  /** SolveToTime(double t, Observer observer, double observer_timestep):
   * Attempts to solve the system up to the given time t, starting from the stored last_time
   *
   * No observer
   *   If the system has a defined stepper controller, it will accelerate/decelerate as needed,
   *    otherwise it will stick to the base timestep of 0.1
   * Observer, no (positive) timestep:
   *    Will run the observer at each timestep dt
   *    As with prev., if there is a controller, this may lead to varying timesteps between observer
   *     calls
   * Observer, (positive) timestep:
   *    Will run the observer at the provided timestep
   *    Between these steps, the base timestep of 0.1 will be used (clamped to the provided step if
   *     it is shorter).
   *    Again, if there is a controller, this will contract/expand this base step as deemed viable
   *     by the controller metrics.
   *    However, the observer is guaranteed to run at the given timestep
   */
  template <class Observer>
  void SolveToTime(double t, Observer observer, double observer_timestep = 0) {
    if (t < this->last_update_time)
      throw std::runtime_error("Attempting to run ODE solver to target time before stored time");

    // Run observer on dynamic timestep, based on solver
    if (observer_timestep <= 0) {
      boost::numeric::odeint::integrate_adaptive(this->system_stepper, this->system,
                                                 this->last_update_state, this->last_update_time, t,
                                                 0.1, observer);
    }
    // Use integrate_cost to have the observer run at a fixed time step
    //  Still allows controlled/error steppers, and fixed step observer for plotting/ recording etc
    else {
      boost::numeric::odeint::integrate_const(this->system_stepper, this->system,
                                              this->last_update_state, this->last_update_time, t,
                                              observer_timestep, observer);
    }
    this->last_update_time = t;
  }
  void SolveToTime(double t) {
    if (t < this->last_update_time)
      throw std::runtime_error("Attempting to run ODE solver to target time before stored time");

    // No need for integrate_const to force observer timestep
    boost::numeric::odeint::integrate_adaptive(
      this->system_stepper, this->system, this->last_update_state, this->last_update_time, t, 0.1);
    this->last_update_time = t;
  }

  double LastTime() { return this->last_update_time; }
  State LastState() { return this->last_update_state; }
};

static void JSON_ParseNumericToDouble(double &out, const boost::json::value *value) {
  switch (value->kind()) {
    case boost::json::kind::int64:
      out = (double)value->as_int64();
      break;
    case boost::json::kind::uint64:
      out = (double)value->as_uint64();
      break;
    case boost::json::kind::double_:
      out = value->as_double();
      break;
    default:
      std::cout << "Invalid JSON value in equation system initialisation - expected numeric type - "
                   "ignoring";
      break;
  }
}

template <class Stepper>
Solver<Stepper, LinearSystem, Vector> LinearSysFromJSON(Stepper stepper,
                                                        boost::json::value system_definition) {
  if (system_definition.kind() != boost::json::kind::object) {
    throw std::runtime_error("Invalid JSON object provided in equation system initialisation\n");
  }
  auto const &system_def_object = system_definition.get_object();
  if (system_def_object.empty())
    throw std::runtime_error("Empty JSON object provided in equation system initialisation\n");

  if (system_def_object.contains("system"))
    std::cout << "WARN - System arguments provided in JSON definition is overriden by system "
                 "argument in function call"
              << std::endl;

  int sys_size = 0;
  boost::json::value val = system_def_object.at("size");
  switch (val.kind()) {
    case boost::json::kind::int64:
      sys_size = val.as_int64();
      break;
    case boost::json::kind::uint64:
      sys_size = val.as_uint64();
      break;
    default:
      throw std::runtime_error("Invalid JSON value for key [size] in equation "
                               "system initialisation\n");
  }
  if (sys_size <= 0)
    throw std::runtime_error("Invalid size for ODE system: " + std::to_string(sys_size) + "\n");

  val = system_def_object.at("A");
  if (val.kind() != boost::json::kind::array)
    throw std::runtime_error("Invalid JSON value for key [A] in equation "
                             "system initialisation\n");
  Matrix A = Matrix(sys_size, sys_size);
  {
    boost::json::array const &arr = val.get_array();
    if (!arr.empty()) {
      auto it = arr.begin();
      int i;
      for (i = 0; i < sys_size * sys_size; ++i) {
        JSON_ParseNumericToDouble(A(i), it);
        if (++it == arr.end())
          break;
      }
      if (i == sys_size * sys_size) {
        std::cout << "WARN - Ignoring values in [A] array in equation system "
                     "initialision due to configured system size"
                  << std::endl;
      }
    }
  }

  val = system_def_object.at("B");
  if (val.kind() != boost::json::kind::array)
    throw std::runtime_error("Invalid JSON value for key [B] in equation "
                             "system initialisation\n");
  Vector B = Vector(sys_size);
  {
    boost::json::array const &arr = val.get_array();
    if (!arr.empty()) {
      auto it = arr.begin();
      int i;
      for (i = 0;; ++i) {
        JSON_ParseNumericToDouble(B(i), it);
        if (++it == arr.end())
          break;
      }
      if (i == sys_size * sys_size) {
        std::cout << "WARN - Ignoring values in [A] array in equation system "
                     "initialision due to configured system size"
                  << std::endl;
      }
    }
  }

  val = system_def_object.at("initial");
  if (val.kind() != boost::json::kind::array)
    throw std::runtime_error("Invalid JSON value for key [initial] in equation "
                             "system initialisation\n");
  Vector x0 = Vector(sys_size);
  {
    boost::json::array const &arr = val.get_array();
    if (!arr.empty()) {
      auto it = arr.begin();
      int i;
      for (i = 0;; ++i) {
        JSON_ParseNumericToDouble(x0(i), it);
        if (++it == arr.end())
          break;
      }
      if (i == sys_size * sys_size) {
        std::cout << "WARN - Ignoring values in [initial] array in equation system "
                     "initialision due to configured system size"
                  << std::endl;
      }
    }
  }

  return Solver<Stepper, LinearSystem, Vector>(stepper, LinearSystem(A, B), x0);
}

Solver<boost::numeric::odeint::controlled_runge_kutta<
         boost::numeric::odeint::runge_kutta_dopri5<Vector>>,
       LinearSystem, Vector>
LinearSysFromJSON(boost::json::value system_definition);

}  // namespace ODE_Solver

#endif