#ifndef STATION_ODE_HPP
#define STATION_ODE_HPP

#include "ode/ode_system.hpp"

class StationSystem;
struct StationSystemInput {
  StationSystem *system;
  double timestep = 1;

  double input_n;

  StationSystemInput(const StationSystemInput &cpy, StationSystem *override = NULL)
      : system(cpy.system), input_n(cpy.input_n) {
    if (override != NULL)
      this->system = override;
  }
  StationSystemInput(StationSystem *system = NULL, int input = -1)
      : system(system), input_n(input) {}

  void operator()(ODE_Solver::Vector &x, double t);
};

class StationSystem : public ODE_Solver::InitialStateSystem<ODE_Solver::Vector> {
  public:
  StationSystem(boost::json::object data);
  // Override copy constructor to make sure the input system pointer updates correctly
  StationSystem(const StationSystem &cpy)
      : segments(cpy.segments), input_segment_index(cpy.input_segment_index) {
    this->input_driver = StationSystemInput(cpy.input_driver, this);
  }

  void operator()(const ODE_Solver::Vector &x, ODE_Solver::Vector &dxdt, const double /* t */);

  // Return the state update vector which should be added to the current state
  ODE_Solver::Vector EntranceUpdate(std::vector<double> n_people);
  StationSystemInput InputDriver() { return this->input_driver; }

  private:
  // Standard pedestrian parameters
  constexpr static double v0 = 1.3;
  constexpr static double p_max = 4;
  constexpr static double t_gap = 0.5;

  constexpr static double l_eff = 1 / p_max;
  constexpr static double p_cap = 1 / (v0 * t_gap + l_eff);

  StationSystemInput input_driver;
  std::vector<int> input_segment_index;

  // --------------------
  //  Flow rate equations
  // --------------------
  static double Qb_out(double p) {
    return v0 * p * (0 <= p && p <= p_cap) + v0 * p_cap * (p > p_cap);
  }
  static double Qb_in(double p) {
    return v0 * p_cap * (0 <= p && p <= p_cap) +
           (1 / t_gap * (1 - p * l_eff)) * (p_cap < p && p <= p_max);
  }
  static double dQe(double p1, double p2, double p3) {
    return fmin(Qb_out(p1), Qb_in(p2)) - fmin(Qb_out(p2), Qb_in(p3));
  }

  static double dQe_split_in(double p1, double p2, double p3, double split) {
    return fmin(Qb_out(p1) * split, Qb_in(p2)) - fmin(Qb_out(p2), Qb_in(p3));
  }

  enum SegmentType : unsigned char {
    INVALID,
    // Straight corridor
    DIRECT,
    // Upstream inflow comes from a junction
    SPLIT_INPUT,
    // Diverging paths at end of segment
    SPLIT_OUTPUT,
    // Platform
    AREA_INFLOW,
    // Entrance
    AREA_OUTFLOW
  };
  enum AreaLink : unsigned char { NONE = 0, FROM = 1, TO = 2, BOTH = FROM | TO };

  struct SegmentData {
    SegmentType type;
    AreaLink linked_to_area;
    unsigned int prev, next;
    double xk;

    // SPLIT_OUTPUT fields
    // These then get read back by SPLIT_INPUT structures in calculations, to avoid data duplication
    unsigned int secondary;
    double split_ratio;

    SegmentData();
    SegmentData(boost::json::object data);
  };
  std::vector<SegmentData> segments;
};

#endif