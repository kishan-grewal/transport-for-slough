#include "ode/ode_solver.hpp"

namespace ODE_Solver {

Solver<boost::numeric::odeint::controlled_runge_kutta<
         boost::numeric::odeint::runge_kutta_dopri5<Vector>>,
       LinearSystem, Vector>
LinearSysFromJSON(boost::json::value system_definition) {
  if (system_definition.kind() != boost::json::kind::object) {
    throw std::runtime_error("Invalid JSON object provided in equation system initialisation\n");
  }
  auto const &system_def_object = system_definition.get_object();
  if (system_def_object.empty())
    throw std::runtime_error("Empty JSON object provided in equation system initialisation\n");

  // Error stepper default values - therefore, if values are undefined, the system will behave as
  // if no controller exists
  double abs_err = 1.0e-6, rel_err = 1.0e-6, a_x = 1, a_dxdt = 1;
  if (system_def_object.contains("stepper")) {
    // No stepper provided explicity, instead construct the stepper from the JSON configuration

    auto stepper_value = system_def_object.at("stepper");
    if (stepper_value.kind() != boost::json::kind::object)
      throw std::runtime_error("Invalid JSON value for key [stepper] in equation system "
                               "initialisation - expected <string> or <object>");
    auto const &stepper_def_object = stepper_value.as_object();

    if (stepper_def_object.contains("abs_err")) {
      auto const &subval = stepper_def_object.at("abs_err");
      JSON_ParseNumericToDouble(abs_err, &subval);
    }
    if (stepper_def_object.contains("rel_err")) {
      auto const &subval = stepper_def_object.at("rel_err");
      JSON_ParseNumericToDouble(rel_err, &subval);
    }
    if (stepper_def_object.contains("a_x")) {
      auto const &subval = stepper_def_object.at("a_x");
      JSON_ParseNumericToDouble(a_x, &subval);
    }
    if (stepper_def_object.contains("a_dxdt")) {
      auto const &subval = stepper_def_object.at("a_dxdt");
      JSON_ParseNumericToDouble(a_dxdt, &subval);
    }
  }

  // std::cout << abs_err << "  " << rel_err << "  " << a_x << "  " << a_dxdt << std::endl;
  return LinearSysFromJSON(
    boost::numeric::odeint::controlled_runge_kutta<
      boost::numeric::odeint::runge_kutta_dopri5<Vector>>(
      boost::numeric::odeint::default_error_checker<double,
                                                    boost::numeric::odeint::vector_space_algebra,
                                                    boost::numeric::odeint::default_operations>(
        abs_err, rel_err, a_x, a_dxdt)),
    system_definition);
}

}  // namespace ODE_Solver
