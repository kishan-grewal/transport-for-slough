#ifndef ODE_SYSTEM_HPP
#define ODE_SYSTEM_HPP

#include <boost/json.hpp>
#include <boost/numeric/odeint.hpp>

#include "ode/json_util.hpp"

namespace ODE_Solver {
typedef typename boost::numeric::ublas::vector<double> Vector;
typedef typename boost::numeric::ublas::matrix<double, boost::numeric::ublas::row_major> Matrix;

class LinearSystem {
  public:
  double input;

  LinearSystem(Matrix A, Vector B, double input = 0) : A(A), B(B), input(input) {}

  void operator()(const Vector &x, Vector &dxdt, const double t) {
    dxdt = boost::numeric::ublas::prod(A, x) + (B * input);
  }

  private:
  Matrix A;
  Vector B;
};

template <class T>
class InitialStateSystem {
  public:
  T get_initialised_state() { return this->initial_state; }

  protected:
  Vector initial_state;
};

class CoefficientSystem {
  public:
  double input;

  CoefficientSystem(boost::json::value values) {
    if (values.is_int64()) {
      coefficients = std::vector<double>(1);
      coefficients[0] = values.as_int64();
    }
    else if (values.is_uint64()) {
      coefficients = std::vector<double>(1);
      coefficients[0] = values.as_uint64();
    }
    else if (values.is_double()) {
      coefficients = std::vector<double>(1);
      coefficients[0] = values.as_double();
    }
    else if (values.is_array()) {
      boost::json::array &arr = values.as_array();
      coefficients = std::vector<double>(arr.size());

      for (int i = 0; i < arr.size(); ++i) {
        JSON_ParseNumericToDouble(coefficients[i], &arr.at(i));
      }
    }
  }

  virtual void operator()(const Vector &x, Vector &dxdt, const double t) = 0;

  protected:
  std::vector<double> coefficients;
};

}  // namespace ODE_Solver

#endif