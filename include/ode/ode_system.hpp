#ifndef ODE_SYSTEM_HPP
#define ODE_SYSTEM_HPP

#include <boost/numeric/odeint.hpp>

namespace ODE_Solver {

class LinearSystem {

public:
  typedef boost::numeric::ublas::vector<double> Vector;
  typedef boost::numeric::ublas::matrix<double,
                                        boost::numeric::ublas::row_major>
      Matrix;

  double input;

  LinearSystem(LinearSystem::Matrix A, LinearSystem::Vector B, double input = 0)
      : A(A), B(B), input(input) {}

  void operator()(const LinearSystem::Vector &x, LinearSystem::Vector &dxdt,
                  const double t) {
    dxdt = boost::numeric::ublas::prod(A, x) + B * input;
  }

private:
  LinearSystem::Matrix A;
  LinearSystem::Vector B;
};

} // namespace ODE_Solver

#endif