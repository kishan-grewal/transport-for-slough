#include <iostream>
#include <vector>

#include <boost/json/src.hpp>
#include <boost/numeric/odeint.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "ode_system.hpp"

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

void pretty_print(std::ostream &os, boost::json::value const &jv,
                  std::string *indent = nullptr) {
  std::string indent_;
  if (!indent)
    indent = &indent_;
  switch (jv.kind()) {
  case boost::json::kind::object: {
    os << "{\n";
    indent->append(4, ' ');
    auto const &obj = jv.get_object();
    if (!obj.empty()) {
      auto it = obj.begin();
      for (;;) {
        os << *indent << boost::json::serialize(it->key()) << " : ";
        pretty_print(os, it->value(), indent);
        if (++it == obj.end())
          break;
        os << ",\n";
      }
    }
    os << "\n";
    indent->resize(indent->size() - 4);
    os << *indent << "}";
    break;
  }

  case boost::json::kind::array: {
    os << "[\n";
    indent->append(4, ' ');
    auto const &arr = jv.get_array();
    if (!arr.empty()) {
      auto it = arr.begin();
      for (;;) {
        os << *indent;
        pretty_print(os, *it, indent);
        if (++it == arr.end())
          break;
        os << ",\n";
      }
    }
    os << "\n";
    indent->resize(indent->size() - 4);
    os << *indent << "]";
    break;
  }

  case boost::json::kind::string: {
    os << boost::json::serialize(jv.get_string());
    break;
  }

  case boost::json::kind::uint64:
  case boost::json::kind::int64:
  case boost::json::kind::double_:
    os << jv;
    break;

  case boost::json::kind::bool_:
    if (jv.get_bool())
      os << "true";
    else
      os << "false";
    break;

  case boost::json::kind::null:
    os << "null";
    break;
  }

  if (indent->empty())
    os << "\n";
}

int main(int argc, char **argv) {

  // state_initialization
  state_type x(2);
  x[0] = 1.0; // start at x=1.0, p=0.0
  x[1] = 0.0;
  // Observed
  std::vector<state_type> x_vec;
  std::vector<double> times;
  // Custom class solver
  boost::numeric::odeint::runge_kutta4<state_type> stepper;
  ODE_SystemSolver<boost::numeric::odeint::runge_kutta4<state_type>, harm_osc,
                   state_type>
      ode_system = ODE_SystemSolver(stepper, harm_osc(0.15), x);

  // Adaptive step (dt is initial step size)
  harm_osc ho(0.15);
  size_t steps = boost::numeric::odeint::integrate(
      ho, x, 0.0, 10.0, 0.1, push_back_state_and_time(x_vec, times));
  std::cout << "Adaptive step step:" << std::endl;
  display(times, x_vec, steps);
  times.clear(), x_vec.clear();

  // Class-based solver
  ode_system.SolveToTime(10);
  std::pair<double, state_type> s = ode_system.LastState();
  std::cout << s.first << "  " << s.second[0] << " " << s.second[1]
            << std::endl;

  //
  // JSON read
  //
  std::ifstream inFile("test.json", std::ios_base::in);
  boost::json::value j = boost::json::parse(inFile);
  inFile.close();
  // pretty_print(std::cout, j);

  boost::numeric::odeint::runge_kutta4<LinearODESystem::Vector>
      linear_ode_stepper;
  auto ss = StateSpaceFromJSON(linear_ode_stepper,
                               j.at("equations").as_array().at(0));
  // ss.system.input = 9.81;

  ss.SolveToTime(10);
  std::cout << ss.LastState().first << "  " << ss.LastState().second[0] << " "
            << ss.LastState().second[1] << std::endl;

  /*
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
  */
}