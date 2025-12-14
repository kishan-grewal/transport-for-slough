#include <iostream>
#include <vector>

#include <boost/json/src.hpp>  // This file MUST only be included once, in all other files, include <boost/json.hpp>
#include <boost/numeric/odeint.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <SFGraphing/PlotDataSet.h>
#include <SFGraphing/SFPlot.h>
#include <SFML/Graphics.hpp>

#include "ode/ode_solver.hpp"
#include "station_ode.hpp"

// rhs_class
/* The rhs of x' = f(x) defined as a class */
class harm_osc {
  double m_gam;

  public:
  harm_osc(double gam) : m_gam(gam) {}

  void operator()(const ODE_Solver::Vector &x, ODE_Solver::Vector &dxdt, const double /* t */) {
    dxdt[0] = x[1];
    dxdt[1] = -x[0] - m_gam * x[1];
  }
};

// Logistic growth:
// dN/dt = rN( 1 - N/k )
class nonlinear_sys_test : ODE_Solver::CoefficientSystem {
  public:
  nonlinear_sys_test(boost::json::value values) : ODE_Solver::CoefficientSystem(values) {}
  void operator()(const ODE_Solver::Vector &x, ODE_Solver::Vector &dxdt, const double t) {
    // Pass
    dxdt[0] = this->coefficients[0] * x[0] * (1 - x[0] / this->coefficients[1]);
  };
};

struct StateTimeObserver {
  std::vector<ODE_Solver::Vector> &m_states;
  std::vector<double> &m_times;

  StateTimeObserver(std::vector<ODE_Solver::Vector> &states, std::vector<double> &times)
      : m_states(states), m_times(times) {}

  void operator()(const ODE_Solver::Vector &x, double t) {
    m_states.push_back(x);
    m_times.push_back(t);
  }
};
struct LinearSystemInput {
  ODE_Solver::LinearSystem &system;
  double timestep = 0.1;

  LinearSystemInput(ODE_Solver::LinearSystem &system) : system(system) {}

  void operator()(ODE_Solver::Vector /*x*/, double t) { system.input = t / 10; };
};

const double t = 50;
void SystemToPlot(csrc::SFPlot &plot, std::vector<ODE_Solver::Vector> &states,
                  std::vector<double> &times, const double y_range[2] = (double[2]){-1, 1},
                  sf::Color colour = sf::Color::Green, std::string label = "") {
  auto sets = csrc::PlotDataSet::MultiPlotDatSet(times, states, colour, csrc::PlottingType::LINE,
                                                 {"x1", "x2"});
  for (auto set = sets.begin(); set != sets.end(); ++set)
    plot.AddDataSet(*set);

  auto minmax = std::minmax_element(times.begin(), times.end());
  plot.SetupAxes(*minmax.first, *minmax.second, y_range[0], y_range[1],
                 (*minmax.second - *minmax.first) / 10, (y_range[1] - y_range[0]) / 10,
                 sf::Color::White);
  plot.GenerateVertices();
}

int main(int argc, char **argv) {
  /*
  std::ifstream station_config_f("config/station_structures.json", std::ios_base::in);
  assert(station_config_f.is_open());
  boost::json::value j = boost::json::parse(station_config_f);
  station_config_f.close();

  std::basic_ifstream<char> station_flows_f("config/station_split_ratios.csv", std::ios_base::in);
  assert(station_flows_f.is_open());

  std::vector<ODE_Solver::Vector> states;
  std::vector<double> times;

  auto station_system = ODE_Solver::Solver<odeint::runge_kutta4<ODE_Solver::Vector>, StationSystem,
                                           ODE_Solver::Vector, StationFileObserver>(
    odeint::runge_kutta4<ODE_Solver::Vector>(),
    StationSystem(j.as_object().at("Canons Park Underground Station").as_object(), station_flows_f),
    StationFileObserver("out/stations/Canons Park Underground Station.csv"));
  station_system.SolveToTime(1000, station_system.system.InputDriver());
  station_system.GetGlobalObserver().finalise();

  station_flows_f.close();
  */

  typedef boost::numeric::odeint::runge_kutta_dopri5<ODE_Solver::Vector> error_stepper_type;
  typedef boost::numeric::odeint::controlled_runge_kutta<error_stepper_type>
    controlled_stepper_type;
  double abs_err = 1.0e-10, rel_err = 1.0e-8, a_x = 0.1, a_dxdt = 0.1;
  controlled_stepper_type controlled_stepper(
    boost::numeric::odeint::default_error_checker<double,
                                                  boost::numeric::odeint::vector_space_algebra,
                                                  boost::numeric::odeint::default_operations>(
      abs_err, rel_err, a_x, a_dxdt));

  std::ifstream station_config_f("test_straight.json", std::ios_base::in);
  assert(station_config_f.is_open());
  boost::json::value j = boost::json::parse(station_config_f);
  station_config_f.close();

  std::basic_ifstream<char> station_flows_f("config/station_split_ratios.csv", std::ios_base::in);
  assert(station_flows_f.is_open());

  std::vector<ODE_Solver::Vector> states;
  std::vector<double> times;

  auto station_system = ODE_Solver::Solver<controlled_stepper_type, StationSystem,
                                           ODE_Solver::Vector, StationFileObserver>(
    controlled_stepper,
    StationSystem(j.as_object().at("stations").as_array().at(0).as_object(), station_flows_f, 1),
    StationFileObserver("out/stations/test.csv"));
  std::cout << "Input: " << station_system.system.InputDriver().timestep
            << " Observer: " << station_system.GetGlobalObserver().timestep << std::endl;
  station_system.SolveToTime(100, station_system.system.InputDriver());
  station_system.GetGlobalObserver().finalise();

  auto vec = station_system.LastState();
  for (auto i = vec.begin(); i != vec.end(); ++i)
    std::cout << *i << ",";
  std::cout << std::endl;

  station_flows_f.close();
}

// sf::RenderWindow window(sf::VideoMode({800, 800}), "ODEint Testing");
// sf::Font font;
// if (!font.openFromFile("/mnt/c/Windows/Fonts/arial.ttf"))
//   throw std::runtime_error("Failed to load font");

// csrc::SFPlot plot(sf::Vector2f(35, 35), sf::Vector2f(700, 700), 35, font, "t", "X");
// double y_range[2] = {0, 50};
// SystemToPlot(plot, states, times, y_range);

// Window rendering
// while (window.isOpen()) {
//   while (const std::optional<sf::Event> event = window.pollEvent()) {
//     if (event->is<sf::Event::Closed>()) {
//       window.close();
//     }
//   }
//   window.clear();
//   window.draw(plot);
//   window.display();
// }