#include <iostream>
#include <vector>

#include <boost/json/src.hpp>  // This file MUST only be included once, in all other files, include <boost/json.hpp>
#include <boost/numeric/odeint.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <SFGraphing/PlotDataSet.h>
#include <SFGraphing/SFPlot.h>
#include <SFML/Graphics.hpp>

#include "ode/ode_solver.hpp"

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

  void operator()(double t) { system.input = t / 10; };
};

struct GlobalStateTimeObserver : ODE_Solver::GlobalTimeObserverTemplate {
  GlobalStateTimeObserver() { this->timestep = 1; }

  virtual void operator()(const ODE_Solver::Vector &x, double t) {
    std::cout << "Step " << t << std::endl;
  };
};

const double t = 50;
void SystemToPlot(csrc::SFPlot &plot, std::vector<ODE_Solver::Vector> &states,
                  std::vector<double> &times, sf::Color colour = sf::Color::Green,
                  std::string label = "") {
  auto sets = csrc::PlotDataSet::MultiPlotDatSet(times, states, colour, csrc::PlottingType::LINE,
                                                 {"x1", "x2"});
  for (auto set = sets.begin(); set != sets.end(); ++set)
    plot.AddDataSet(*set);

  auto minmax = std::minmax_element(times.begin(), times.end());
  plot.SetupAxes(*minmax.first, *minmax.second, -5, 5, (*minmax.second - *minmax.first) / 10, 0.5,
                 sf::Color::White);
  plot.GenerateVertices();
}

int main(int argc, char **argv) {
  sf::RenderWindow window(sf::VideoMode(800, 800), "ODEint Testing");
  sf::Font font;
  font.loadFromFile("/mnt/c/Windows/Fonts/arial.ttf");

  //
  // Manual equation definition
  //

  // state_initialization
  ODE_Solver::Vector x(2);
  x[0] = 1.0, x[1] = 0.0;  // start at x=1.0, p=0.0

  // Custom class solver
  ODE_Solver::Solver<odeint::runge_kutta4<ODE_Solver::Vector>, harm_osc, ODE_Solver::Vector,
                     GlobalStateTimeObserver>
    ode_system = ODE_Solver::Solver(odeint::runge_kutta4<ODE_Solver::Vector>(), harm_osc(0.15), x,
                                    GlobalStateTimeObserver());
  ode_system.SolveToTime(0.1);
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

  // Window rendering
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }
    window.clear();
    // window.draw(plot);
    // window.draw(plot2);
    window.draw(plot3);
    window.display();
  }
}