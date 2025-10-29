#ifndef ODE_SYSTEM_HPP
#define ODE_SYSTEM_HPP

#include <boost/numeric/odeint.hpp>

template <class Stepper, class System, class State> class ODE_System {
private:
  Stepper system_stepper;
  System equation_system;

  double last_update_time;
  State last_update_state;

public:
  ODE_System(Stepper stepper, System system, State state)
      : system_stepper(stepper), equation_system(system),
        last_update_state(state) {
    this->last_update_time = 0;
  };
  // ~ODE_System();

  void SolveToTime(double t) {
    boost::numeric::odeint::integrate_adaptive(
        this->system_stepper, this->equation_system, this->last_update_state,
        this->last_update_time, t, 0.01);
    this->last_update_time = t;
  }
  std::pair<double, State> LastState() {
    return std::pair<double, State>(this->last_update_time,
                                    this->last_update_state);
  }
};

#endif