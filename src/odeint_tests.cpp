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

double input_noise, time_offset, sim_time;
bool settings_loaded = false;
void load_settings() {
  std::ifstream conf("config/config.json", std::ios_base::in);
  assert(conf.is_open());
  boost::json::object j = boost::json::parse(conf).as_object();
  conf.close();

  JSON_ParseNumericToDouble(input_noise, &j.at("input_noise"));
  JSON_ParseNumericToDouble(time_offset, &j.at("time_offset"));
  JSON_ParseNumericToDouble(sim_time, &j.at("sim_time"));

  settings_loaded = true;
}

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

namespace odeint = boost::numeric::odeint;
int main(int argc, char **argv) {
  load_settings();
  if (!settings_loaded)
    throw std::runtime_error("Settings not loaded");

  std::ifstream station_config_f("config/station_structures.json");
  assert(station_config_f.is_open());
  boost::json::value j = boost::json::parse(station_config_f);
  station_config_f.close();

  std::basic_ifstream<char> station_flow_splits_f("config/station_split_ratios.csv");
  assert(station_flow_splits_f.is_open());

  std::basic_ifstream<char> station_flows_f("config/station_entrance_flows.csv");
  assert(station_flows_f.is_open());

  std::vector<ODE_Solver::Vector> states;
  std::vector<double> times;

  typedef odeint::runge_kutta_dopri5<ODE_Solver::Vector> error_stepper_type;
  typedef odeint::controlled_runge_kutta<error_stepper_type> controlled_stepper_type;
  double abs_err = 1.0e-10, rel_err = 1.0e-8, a_x = 0.1, a_dxdt = 0.1;
  controlled_stepper_type controlled_stepper(
    odeint::default_error_checker<double, odeint::vector_space_algebra, odeint::default_operations>(
      abs_err, rel_err, a_x, a_dxdt));

  auto station_system = ODE_Solver::Solver<odeint::runge_kutta4<ODE_Solver::Vector>, StationSystem,
                                           ODE_Solver::Vector, StationFileObserver>(
    odeint::runge_kutta4<ODE_Solver::Vector>(),
    StationSystem(j.as_object().at("Bond Street Underground Station").as_object(),
                  station_flow_splits_f, station_flows_f),
    StationFileObserver("out/stations/Bond Street Underground Station.csv", 5));
  // station_system.LastState() += station_system.system.PlatformUpdateVector(5, 0);

  station_system.SolveToTime(sim_time, station_system.system.InputDriver());  //
  station_system.GetGlobalObserver().finalise();

  station_flow_splits_f.close();

  /*
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
*/
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