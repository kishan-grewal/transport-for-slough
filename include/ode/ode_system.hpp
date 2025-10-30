#ifndef ODE_SYSTEM_HPP
#define ODE_SYSTEM_HPP

#include <boost/numeric/odeint.hpp>

namespace ODE_Solver {
typedef typename boost::numeric::ublas::vector<double> Vector;
typedef typename boost::numeric::ublas::matrix<double, boost::numeric::ublas::row_major> Matrix;

class LinearSystem {
  public:
  double input;

  LinearSystem(Matrix A, Vector B, double input = 0) : A(A), B(B), input(input) {}

  void operator()(const Vector &x, Vector &dxdt, const double t) {
    dxdt = boost::numeric::ublas::prod(A, x) + B * input;
  }

  private:
  Matrix A;
  Vector B;
};

}  // namespace ODE_Solver

#endif