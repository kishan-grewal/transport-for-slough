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

namespace odeint = boost::numeric::odeint;

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

struct GlobalStateTimeObserver : ODE_Solver::GlobalTimeObserverTemplate {
  std::vector<ODE_Solver::Vector> &m_states;
  std::vector<double> &m_times;

  GlobalStateTimeObserver(std::vector<ODE_Solver::Vector> &states, std::vector<double> &times,
                          double timestep = 1)
      : m_states(states), m_times(times) {
    this->timestep = timestep;
  }

  virtual void operator()(const ODE_Solver::Vector &x, double t) {
    // std::cout << "Step " << t << std::endl;
    m_states.push_back(x);
    m_times.push_back(t);
  };
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

/*
int main(int argc, char **argv) {
  sf::RenderWindow window(sf::VideoMode({800, 800}), "ODEint Testing");
  sf::Font font;
  if (!font.openFromFile("/mnt/c/Windows/Fonts/arial.ttf"))
    throw std::runtime_error("Failed to load font");

  //
  // Manual equation definition
  //

  // state_initialization
  ODE_Solver::Vector x(2);
  x[0] = 1.0, x[1] = 0.0;  // start at x=1.0, p=0.0

  // Custom class solver
  auto ode_system =
    ODE_Solver::Solver(odeint::runge_kutta4<ODE_Solver::Vector>(), harm_osc(0.15), x);
  ode_system.SolveToTime(3.6745);
  ode_system.SolveToTime(t);
  ODE_Solver::Vector s = ode_system.LastState();
  std::cout << "Harmonic Osc. ending state:" << ode_system.LastTime() << "  " << s[0] << " " << s[1]
            << std::endl;

  //
  // JSON equation defintion
  //
  std::ifstream inFile("test.json", std::ios_base::in);
  boost::json::value j = boost::json::parse(inFile);
  inFile.close();

  // --------------------------------------
  // Solver class testing
  // --------------------------------------
  auto const &equation_definition_array = j.at("equations").as_array();

  auto ss = ODE_Solver::LinearSysFromJSON(odeint::runge_kutta_dopri5<ODE_Solver::Vector>(),
                                          equation_definition_array.at(0));
  auto ss2 = ODE_Solver::LinearSysFromJSON(equation_definition_array.at(1));
  ss2.system.input = 0.05;

  // Solving
  std::vector<ODE_Solver::Vector> states;
  std::vector<double> times;

  ss.SolveToTime(t, StateTimeObserver(states, times), -1, LinearSystemInput(ss.system));
  csrc::SFPlot plot1(sf::Vector2f(0, 0), sf::Vector2f(375, 700), 35, font, "t", "X");
  SystemToPlot(plot1, states, times);
  states.clear(), times.clear();

  ss2.SolveToTime(t, StateTimeObserver(states, times), -1);
  csrc::SFPlot plot2(sf::Vector2f(400, 0), sf::Vector2f(375, 700), 35, font, "t", "X");
  SystemToPlot(plot2, states, times, sf::Color::Blue);
  states.clear(), times.clear();

  //
  // Nonlinear equation defintion (JSON coefficients)
  //
  auto nonlinear_state = ODE_Solver::Vector(1);
  nonlinear_state[0] = 5;
  auto nonlinear = ODE_Solver::Solver<
    odeint::controlled_runge_kutta<odeint::runge_kutta_dopri5<ODE_Solver::Vector>>,
    nonlinear_sys_test, ODE_Solver::Vector>(
    odeint::controlled_runge_kutta<odeint::runge_kutta_dopri5<ODE_Solver::Vector>>(),
    nonlinear_sys_test(j.at("coefficients").as_array().at(0)), nonlinear_state);
  nonlinear.SolveToTime(t, StateTimeObserver(states, times), 0.1);
  csrc::SFPlot plot3(sf::Vector2f(0, 0), sf::Vector2f(700, 700), 35, font, "t", "X");
  SystemToPlot(plot3, states, times, sf::Color::Blue);
  states.clear(), times.clear();

  //
  // Faking a DDE with the ODE solver
  //
  auto dde_state = ODE_Solver::Vector(2);
  dde_state[0] = 0, dde_state[1] = 0;
  auto dde_sys = dde_sys_test(60, dde_state);
  dde_state[0] = 10, dde_state[1] = 0;

  auto dde = ODE_Solver::Solver<
    odeint::controlled_runge_kutta<odeint::runge_kutta_dopri5<ODE_Solver::Vector>>, dde_sys_test,
    ODE_Solver::Vector, GlobalStateTimeObserver>(
    odeint::controlled_runge_kutta<odeint::runge_kutta_dopri5<ODE_Solver::Vector>>(), dde_sys,
    dde_state, GlobalStateTimeObserver(states, times));
  auto dde_driver = dde_sys_test_driver(&dde.system.past_states, &dde.system.memory_ptr, 60);
  dde.SolveToTime(100, dde_driver);
  csrc::SFPlot plot4(sf::Vector2f(0, 0), sf::Vector2f(700, 700), 35, font, "t", "X");
  SystemToPlot(plot4, states, times, sf::Color::Blue);
  states.clear(), times.clear();
  std::cout << dde.system.memory_ptr << std::endl;

  // Window rendering
  while (window.isOpen()) {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }
    }
    window.clear();
    // window.draw(plot1);
    // window.draw(plot2);

    // window.draw(plot3);

    window.draw(plot4);
    window.display();
  }
}
*/

int main(int argc, char **argv) {
  sf::RenderWindow window(sf::VideoMode({800, 800}), "ODEint Testing");
  sf::Font font;
  if (!font.openFromFile("/mnt/c/Windows/Fonts/arial.ttf"))
    throw std::runtime_error("Failed to load font");

  std::ifstream inFile("test.json", std::ios_base::in);
  boost::json::value j = boost::json::parse(inFile);
  inFile.close();

  std::vector<ODE_Solver::Vector> states;
  std::vector<double> times;

  auto station_system = ODE_Solver::Solver<odeint::runge_kutta4<ODE_Solver::Vector>, StationSystem,
                                           ODE_Solver::Vector, GlobalStateTimeObserver>(
    odeint::runge_kutta4<ODE_Solver::Vector>(),
    StationSystem(j.as_object().at("stations").as_array().at(0).as_object()),
    GlobalStateTimeObserver(states, times));
  station_system.SolveToTime(150, station_system.system.InputDriver());
  // station_system.LastState() += station_system.system.PlatformUpdateVector(-10, 0);
  // station_system.LastState() += station_system.system.PlatformUpdateVector(15, 0);
  station_system.SolveToTime(200, station_system.system.InputDriver());
  // ODE_Solver::Vector s = station_system.LastState();
  // for (int i = 0; i < s.size(); ++i) {
  //   std::cout << s[i] << " ";
  // }
  // std::cout << std::endl;
  for (int i = 0; i < states.size(); ++i) {
    double s = 0;
    for (int j = 0; j < states[i].size(); ++j) {
      std::cout << std::fixed << std::setprecision(2) << states[i][j] << " ";
      s += states[i][j];

      if (j != 0 && j != 5 && j != 6 && j != 11)
        s += states[i][j];
    }
    // std::cout << "\t\t" << s << std::endl;
    std::cout << std::endl;
  }

  csrc::SFPlot plot(sf::Vector2f(35, 35), sf::Vector2f(700, 700), 35, font, "t", "X");
  double y_range[2] = {0, 50};
  SystemToPlot(plot, states, times, y_range);

  // Window rendering
  while (window.isOpen()) {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }
    }
    window.clear();
    window.draw(plot);
    window.display();
  }
}