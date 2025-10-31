#include <iostream>
#include <vector>

#include <boost/json/src.hpp>  // This file MUST only be included once, in all other files, include <boost/json.hpp>
#include <boost/numeric/odeint.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <SFML/Graphics.hpp>
#include <SFGraphing/SFPlot.h>
#include <SFGraphing/PlotDataSet.h>

#include "ode/ode_solver.hpp"

// rhs_function
/* The type of container used to hold the state vector */
typedef std::vector<double> state_type;

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

void display(std::vector<double> times, std::vector<state_type> state, size_t steps) {
  for (size_t i = 0; i <= steps; i++) {
    std::cout << std::fixed << std::setprecision(5) << times[i] << " " << state[i][0] << " "
              << state[i][1] << '\n';
  }
}

void pretty_print(std::ostream &os, boost::json::value const &jv, std::string *indent = nullptr) {
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

void SystemToPlot(csrc::SFPlot &plot, std::vector<ODE_Solver::Vector> &states,
                  std::vector<double> &times, sf::Color colour = sf::Color::Green,
                  std::string label = "") {
  auto sets = csrc::PlotDataSet::MultiPlotDatSet(times, states, colour, csrc::PlottingType::LINE,
                                                 {"x1", "x2"});
  for (auto set = sets.begin(); set != sets.end(); ++set)
    plot.AddDataSet(*set);

  plot.SetupAxes(*std::min_element(times.begin(), times.end()),
                 *std::max_element(times.begin(), times.end()), -2, 2, 1, 0.5, sf::Color::White);
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
  state_type x(2);
  x[0] = 1.0;  // start at x=1.0, p=0.0
  x[1] = 0.0;

  // Custom class solver
  ODE_Solver::Solver<boost::numeric::odeint::runge_kutta4<state_type>, harm_osc, state_type>
    ode_system =
      ODE_Solver::Solver(boost::numeric::odeint::runge_kutta4<state_type>(), harm_osc(0.15), x);
  ode_system.SolveToTime(10);
  state_type s = ode_system.LastState();
  std::cout << "Ending state:" << ode_system.LastTime() << "  " << s[0] << " " << s[1] << std::endl;

  //
  // JSON equation defintion
  //
  std::ifstream inFile("test.json", std::ios_base::in);
  boost::json::value j = boost::json::parse(inFile);
  inFile.close();
  // pretty_print(std::cout, j);

  // --------------------------------------
  // Solver class testing
  // --------------------------------------
  auto const &equation_array = j.at("equations").as_array();

  auto ss = ODE_Solver::LinearSysFromJSON(
    boost::numeric::odeint::runge_kutta_dopri5<ODE_Solver::Vector>(), equation_array.at(0));
  auto ss2 = ODE_Solver::LinearSysFromJSON(equation_array.at(0));

  // Solving
  std::vector<ODE_Solver::Vector> states;
  std::vector<double> times;

  ss.SolveToTime(10, StateTimeObserver(states, times), -1, LinearSystemInput(ss.system));
  csrc::SFPlot plot(sf::Vector2f(0, 0), sf::Vector2f(300, 700), 50, font, "t", "X");
  SystemToPlot(plot, states, times);
  states.clear(), times.clear();

  ss2.SolveToTime(10, StateTimeObserver(states, times), -1);
  csrc::SFPlot plot2(sf::Vector2f(350, 0), sf::Vector2f(300, 700), 50, font, "t", "X");
  SystemToPlot(plot2, states, times, sf::Color::Blue);
  states.clear(), times.clear();

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }
    window.clear();
    window.draw(plot);
    window.draw(plot2);
    window.display();
  }
}