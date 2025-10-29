#include <iostream>
#include <vector>

#include <boost/numeric/odeint.hpp>

// rhs_function
/* The type of container used to hold the state vector */
typedef std::vector<double> state_type;

const double gam = 0.15;

/* The rhs of x' = f(x) */
void harmonic_oscillator(const state_type &x, state_type &dxdt,
                         const double /* t */) {
  dxdt[0] = x[1];
  dxdt[1] = -x[0] - gam * x[1];
}

// rhs_class
/* The rhs of x' = f(x) defined as a class */
class harm_osc {

  double m_gam;

public:
  harm_osc(double gam) : m_gam(gam) {}

  void operator()(const state_type &x, state_type &dxdt, const double /* t */) {
    dxdt[0] = x[1];
    dxdt[1] = -x[0] - m_gam * x[1];
  }
};

// integrate_observer
struct push_back_state_and_time {
  std::vector<state_type> &m_states;
  std::vector<double> &m_times;

  push_back_state_and_time(std::vector<state_type> &states,
                           std::vector<double> &times)
      : m_states(states), m_times(times) {}

  void operator()(const state_type &x, double t) {
    m_states.push_back(x);
    m_times.push_back(t);
  }
};

void display(std::vector<double> times, std::vector<state_type> state,
             size_t steps) {
  for (size_t i = 0; i <= steps; i++) {
    std::cout << std::fixed << std::setprecision(5) << times[i] << " "
              << state[i][0] << " " << state[i][1] << '\n';
  }
}

int main(int /* argc */, char ** /* argv */) {

  // state_initialization
  state_type x(2);
  x[0] = 1.0; // start at x=1.0, p=0.0
  x[1] = 0.0;

  // Observed
  std::vector<state_type> x_vec;
  std::vector<double> times;

  // Adaptive step (dt is initial step size)
  harm_osc ho(0.15);
  size_t steps = boost::numeric::odeint::integrate(
      ho, x, 0.0, 10.0, 0.1, push_back_state_and_time(x_vec, times));
  std::cout << "Adaptive step step:" << std::endl;
  display(times, x_vec, steps);
  times.clear(), x_vec.clear();

  // Constant step
  boost::numeric::odeint::runge_kutta4<state_type> stepper;
  steps = boost::numeric::odeint::integrate_const(
      stepper, harmonic_oscillator, x, 0.0, 10.0, 0.05,
      push_back_state_and_time(x_vec, times));
  std::cout << "Constant step:" << std::endl;
  display(times, x_vec, steps);
  times.clear(), x_vec.clear();

  // Constant step in loop
  const double dt = 0.05;
  steps = 0;
  state_type x_tmp = x;
  std::cout << "Constant step:" << std::endl;
  for (double t = 0.0; t < 10.0; t += dt, ++steps) {
    stepper.do_step(harmonic_oscillator, x_tmp, t, dt);
    std::cout << std::fixed << std::setprecision(5) << t << " " << x_tmp[0]
              << " " << x_tmp[1] << '\n';
  }

  //[ define_adapt_stepper
  typedef boost::numeric::odeint::runge_kutta_cash_karp54<state_type>
      error_stepper_type;

  //[ integrate_adapt
  typedef boost::numeric::odeint::controlled_runge_kutta<error_stepper_type>
      controlled_stepper_type;
  controlled_stepper_type controlled_stepper;
  integrate_adaptive(controlled_stepper, harmonic_oscillator, x, 0.0, 10.0,
                     0.01);
  //]

  {
    //[integrate_adapt_full
    double abs_err = 1.0e-10, rel_err = 1.0e-6, a_x = 1.0, a_dxdt = 1.0;
    controlled_stepper_type controlled_stepper(
        boost::numeric::odeint::default_error_checker<
            double, boost::numeric::odeint::range_algebra,
            boost::numeric::odeint::default_operations>(abs_err, rel_err, a_x,
                                                        a_dxdt));
    integrate_adaptive(controlled_stepper, harmonic_oscillator, x, 0.0, 10.0,
                       0.01);
    //]
  }

  //[integrate_adapt_make_controlled
  integrate_adaptive(
      boost::numeric::odeint::make_controlled<error_stepper_type>(1.0e-10,
                                                                  1.0e-6),
      harmonic_oscillator, x, 0.0, 10.0, 0.01);
  //]

  //[integrate_adapt_make_controlled_alternative
  integrate_adaptive(make_controlled(1.0e-10, 1.0e-6, error_stepper_type()),
                     harmonic_oscillator, x, 0.0, 10.0, 0.01);
  //]

  // #ifdef BOOST_NUMERIC_ODEINT_CXX11
  //   //[ define_const_stepper_cpp11
  //   {
  //     runge_kutta4<state_type> stepper;
  //     integrate_const(
  //         stepper,
  //         [](const state_type &x, state_type &dxdt, double t) {
  //           dxdt[0] = x[1];
  //           dxdt[1] = -x[0] - gam * x[1];
  //         },
  //         x, 0.0, 10.0, 0.01);
  //   }
  //   //]

  //   //[ harm_iterator_const_step]
  //   std::for_each(
  //       make_const_step_time_iterator_begin(stepper, harmonic_oscillator, x,
  //       0.0,
  //                                           0.1, 10.0),
  //       make_const_step_time_iterator_end(stepper, harmonic_oscillator, x),
  //       [](std::pair<const state_type &, const double &> x) {
  //         std::cout << x.second << " " << x.first[0] << " " << x.first[1] <<
  //         "\n";
  //       });
  // //]
  // #endif
}