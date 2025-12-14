#ifndef ODE_SOLVER_HPP
#define ODE_SOLVER_HPP

#include <boost/json.hpp>
#include <boost/numeric/odeint.hpp>

#include <stdexcept>

#include "ode/json_util.hpp"
#include "ode/ode_system.hpp"

namespace ODE_Solver {

class EmptyObserver {};
class GlobalTimeObserverTemplate {
  public:
  double timestep;
  bool skip_next = false;

  virtual void operator()(const ODE_Solver::Vector &x, double t) = 0;
};

template <class Stepper, class System, class State, class GlobalObserver = EmptyObserver>
class Solver {
  private:
  Stepper system_stepper;
  GlobalObserver global_time_observer;

  double last_update_time;
  State last_update_state;

  public:
  System system;

  // Using std::forward<T> to allow use of both lvalues and rvalues in the constructure for the
  // system - this means you can both directly instantiate the system in the constructor, OR use a
  // reference to it (where a standard copy constructor would then break the reference)
  explicit Solver(Stepper stepper, System &&system, GlobalObserver &&observer = EmptyObserver())
      : system_stepper(stepper),
        system(std::forward<System>(system)),
        last_update_state(system.get_initialised_state()),
        global_time_observer(observer) {
    this->last_update_time = 0;
  };
  explicit Solver(Stepper stepper, System &&system, State state,
                  GlobalObserver observer = EmptyObserver())
      : system_stepper(stepper),
        system(std::forward<System>(system)),
        last_update_state(state),
        global_time_observer(observer) {
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
  void SolveToTime(double t, Observer observer, double observer_timestep) {
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

    if constexpr (std::is_same<GlobalObserver, EmptyObserver>::value) {
      // No need for integrate_const to force observer timestep
      boost::numeric::odeint::integrate_adaptive(this->system_stepper, this->system,
                                                 this->last_update_state, this->last_update_time, t,
                                                 0.1);
      this->last_update_time = t;
    }
    else {
      // this->global_time_observer.skip_next = false;
      if (std::fmod(this->last_update_time, this->global_time_observer.timestep) != 0) {
        // Fast-forward the simulation to the next observer timestep, without observation
        double new_time = (int)(this->last_update_time / this->global_time_observer.timestep) + 1;
        new_time = std::clamp(new_time * this->global_time_observer.timestep, 0.0, t);
        std::cout << "Fast forward from " << this->last_update_time << " to " << new_time
                  << std::endl;

        boost::numeric::odeint::integrate_adaptive(this->system_stepper, this->system,
                                                   this->last_update_state, this->last_update_time,
                                                   new_time, 0.1);
        this->last_update_time = new_time;

        if (this->last_update_time < t)
          std::cout << "\t";

        this->global_time_observer.skip_next = false;
      }
      // Last update was already observed by a previous SolveToTime call
      // else if (this->last_update_time < t and this->last_update_time != 0)
      //   this->global_time_observer.skip_next = true;

      if (this->last_update_time < t) {
        std::cout << "Running observer from " << this->last_update_time << " to " << t << std::endl;

        boost::numeric::odeint::integrate_const(
          this->system_stepper, this->system, this->last_update_state, this->last_update_time, t,
          this->global_time_observer.timestep, this->global_time_observer);
        this->last_update_time = t;

        if (fmod(this->last_update_time, this->global_time_observer.timestep) == 0)
          this->global_time_observer.skip_next = true;
      }
    }
  }

  template <class Observer, class InputDriver>
  void SolveToTime(double t, Observer observer, double observer_timestep, InputDriver input) {
    if (t < this->last_update_time)
      throw std::runtime_error("Attempting to run ODE solver to target time before stored time");
    if (input.timestep <= 0)
      throw std::runtime_error(
        "Attempting to run ODE solver with an invalid input timestep - must be > 0");

    double tmp_t = this->last_update_time;
    input(last_update_state, tmp_t);  // Initialise input signal

    while (tmp_t < t) {
      // Update internal time
      tmp_t = std::clamp(tmp_t + input.timestep, 0.0, t);
      this->SolveToTime(tmp_t, observer, observer_timestep);

      input(last_update_state, tmp_t);  // Update input signal after running solver
    }
    this->last_update_time = t;
  }

  template <class InputDriver>
  void SolveToTime(double t, InputDriver input) {
    if (t < this->last_update_time)
      throw std::runtime_error("Attempting to run ODE solver to target time before stored time");
    if (input.timestep <= 0)
      throw std::runtime_error(
        "Attempting to run ODE solver with an invalid input timestep - must be > 0");

    double tmp_t = this->last_update_time;

    if (fmod(this->last_update_time, input.timestep) != 0) {
      tmp_t = std::ceil(this->last_update_time / input.timestep) * input.timestep;
      this->SolveToTime(tmp_t);
    }
    while (tmp_t < t) {
      input(last_update_state, this->last_update_time);  // Update input signal after running solver

      // Update internal time
      tmp_t = std::clamp(this->last_update_time + input.timestep, 0.0, t);
      this->SolveToTime(tmp_t);
    }
    // this->last_update_time = t;
  }

  double LastTime() { return this->last_update_time; }
  State &LastState() { return this->last_update_state; }
  inline GlobalObserver GetGlobalObserver() { return global_time_observer; }
};

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

// Deduction guide
//  For InitialStateSystem which contains the initial state internally
template <class Stepper, class GlobalObserver, class State>
Solver(Stepper stepper, InitialStateSystem<State> &&system,
       GlobalObserver observer = EmptyObserver())
  -> Solver<Stepper, InitialStateSystem<State>, State, GlobalObserver>;

}  // namespace ODE_Solver

#endif