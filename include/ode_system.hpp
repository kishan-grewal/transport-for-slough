#ifndef ODE_SYSTEM_HPP
#define ODE_SYSTEM_HPP

#include <boost/json/src.hpp>
#include <boost/numeric/odeint.hpp>

#include <stdexcept>

class LinearODESystem {

public:
  typedef boost::numeric::ublas::vector<double> Vector;
  typedef boost::numeric::ublas::matrix<double,
                                        boost::numeric::ublas::row_major>
      Matrix;

  double input;

  LinearODESystem(LinearODESystem::Matrix A, LinearODESystem::Vector B,
                  double input = 0)
      : A(A), B(B), input(input) {}

  void operator()(const LinearODESystem::Vector &x,
                  LinearODESystem::Vector &dxdt, const double t) {
    dxdt = boost::numeric::ublas::prod(A, x) + B * input;
  }

private:
  LinearODESystem::Matrix A;
  LinearODESystem::Vector B;
};

template <class Stepper, class System, class State> class ODE_SystemSolver {
private:
  Stepper system_stepper;

  double last_update_time;
  State last_update_state;

public:
  System system;

  ODE_SystemSolver(Stepper stepper, System system, State state)
      : system_stepper(stepper), system(system), last_update_state(state) {
    this->last_update_time = 0;
  };
  // ~ODE_System();

  void SolveToTime(double t) {
    boost::numeric::odeint::integrate_adaptive(
        this->system_stepper, this->system, this->last_update_state,
        this->last_update_time, t, 0.01);
    this->last_update_time = t;
  }
  std::pair<double, State> LastState() {
    return std::pair<double, State>(this->last_update_time,
                                    this->last_update_state);
  }
};

static void JSON_ParseToDouble(double &out, const boost::json::value *value) {
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
    std::cout << "Invalid JSON value in array [A] in equation system "
                 "initialisation - ignoring";
    break;
  }
}

template <class Stepper>
ODE_SystemSolver<Stepper, LinearODESystem, LinearODESystem::Vector>
StateSpaceFromJSON(Stepper stepper, boost::json::value system_definition) {
  if (system_definition.kind() != boost::json::kind::object) {
    throw std::runtime_error(
        "Invalid JSON object provided in equation system initialisation\n");
  }
  auto const &system_def_object = system_definition.get_object();
  if (system_def_object.empty())
    throw std::runtime_error(
        "Empty JSON object provided in equation system initialisation\n");

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
    throw std::runtime_error(
        "Invalid size for ODE system: " + std::to_string(sys_size) + "\n");

  val = system_def_object.at("A");
  if (val.kind() != boost::json::kind::array)
    throw std::runtime_error("Invalid JSON value for key [A] in equation "
                             "system initialisation\n");
  LinearODESystem::Matrix A = LinearODESystem::Matrix(sys_size, sys_size);
  {
    boost::json::array const &arr = val.get_array();
    if (!arr.empty()) {
      auto it = arr.begin();
      int i;
      for (i = 0; i < sys_size * sys_size; ++i) {
        JSON_ParseToDouble(A(i), it);
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
  LinearODESystem::Vector B = LinearODESystem::Vector(sys_size);
  {
    boost::json::array const &arr = val.get_array();
    if (!arr.empty()) {
      auto it = arr.begin();
      int i;
      for (i = 0;; ++i) {
        JSON_ParseToDouble(B(i), it);
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
  LinearODESystem::Vector x0 = LinearODESystem::Vector(sys_size);
  {
    boost::json::array const &arr = val.get_array();
    if (!arr.empty()) {
      auto it = arr.begin();
      int i;
      for (i = 0;; ++i) {
        JSON_ParseToDouble(x0(i), it);
        if (++it == arr.end())
          break;
      }
      if (i == sys_size * sys_size) {
        std::cout
            << "WARN - Ignoring values in [initial] array in equation system "
               "initialision due to configured system size"
            << std::endl;
      }
    }
  }

  return ODE_SystemSolver<Stepper, LinearODESystem, LinearODESystem::Vector>(
      stepper, LinearODESystem(A, B), x0);
}

#endif