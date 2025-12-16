#ifndef STATION_ODE_HPP
#define STATION_ODE_HPP
#include <fstream>
#include <boost/tokenizer.hpp>

#include "ode/ode_solver.hpp"
#include "ode/ode_system.hpp"

struct StationFileObserverCache {
  constexpr static unsigned int CACHE_SIZE = 50;
  StationFileObserverCache(std::string path) : f(path), fpath(path), cache(50), cache_ptr(0) {
    if (!f.is_open())
      throw std::runtime_error("WARN - failed to open logging file [" + path + "]");
  };
  ~StationFileObserverCache() {
    if (f.is_open())
      f.close();
  }

  void push(ODE_Solver::Vector x) {
    cache[cache_ptr++] = ODE_Solver::Vector(x);
    if (cache_ptr == CACHE_SIZE)
      fdump();
  }

  void finalise() {
    // Dump anything remaining in cache
    if (!f.is_open()) {
      std::cout << "No file to finalise to" << std::endl;
      return;
    }
    for (int i = 0; i < cache_ptr; ++i) {
      for (auto val = cache[i].begin(); val != cache[i].end(); ++val)
        f << *val << ",";
      f << std::endl;
    }
    f.close();
  }

  private:
  std::ofstream f;
  std::string fpath;
  std::vector<ODE_Solver::Vector> cache;
  unsigned int cache_ptr = 0;

  void fdump() {
    for (auto vec = cache.begin(); vec != cache.end(); ++vec) {
      for (auto val = vec->begin(); val != vec->end(); ++val)
        f << std::fixed << std::setprecision(5) << *val << ",";
      f << std::endl;
    }
    cache_ptr = 0;
  }
};

class StationSystem;
struct StationSystemInput {
  double timestep = 1;

  StationSystem *system;
  double input_n = -1;  // Const input, used for testing (default to negative, which is ignored)

  StationSystemInput(const StationSystemInput &cpy, StationSystem *override = NULL)
      : system(cpy.system),
        input_n(cpy.input_n),
        timestep(cpy.timestep),

        last_update_t(cpy.last_update_t),
        target_inputs(cpy.target_inputs),
        accumulated(cpy.accumulated),
        inflows(cpy.inflows_path),
        inflows_path(cpy.inflows_path),

        cache(cpy.cache) {
    if (override != NULL)
      this->system = override;
  }
  StationSystemInput(std::string log_path, StationSystem *system = NULL, int input = -1);

  void operator()(ODE_Solver::Vector &x, double t);
  void initialise_timeseries_input(int entrance_count, std::string flows);
  void log_finalise();

  private:
  int last_update_t;
  std::shared_ptr<StationFileObserverCache> cache;

  constexpr static int TIME_SERIES_TIMESTEP = 15 * 60;
  // constexpr static int TIME_SERIES_ENTRY_COUNT = (24 * 60) / 15;  // 15 minute slices across a
  // day
  constexpr static int TIME_SERIES_ROW_OFFSET = 1;  // Skip column headers
  constexpr static int TIME_SERIES_MAX_LINE_LEN = 2000;

  std::vector<double> accumulated;
  std::vector<double> target_inputs;
  // std::vector<double> interp_t;

  std::ifstream inflows;
  std::string inflows_path;
  double read_inflow(int flow_index, double t);
};

struct StationFileObserver : ODE_Solver::GlobalTimeObserverTemplate {
  StationFileObserver(std::string path, double timestep = 5)
      : cache(std::make_shared<StationFileObserverCache>(path)) {
    this->timestep = timestep;
  }
  StationFileObserver(StationFileObserver &cpy) : cache(cpy.cache) {
    this->timestep = cpy.timestep;
    this->skip_next = cpy.skip_next;
  };

  void operator()(const ODE_Solver::Vector &x, double t) {
    if (this->skip_next) {
      this->skip_next = false;
      return;
    }
    cache->push(x);
  };
  void log_finalise() { cache->finalise(); }

  private:
  std::shared_ptr<StationFileObserverCache> cache;
};

class StationSystem : public ODE_Solver::InitialStateSystem<ODE_Solver::Vector> {
  public:
  StationSystem(boost::json::object data, std::string split_ratios, std::string flows,
                double input_timestep = 1, std::string flow_logging = "");
  // Override copy constructor to make sure the input system pointer updates correctly
  StationSystem(const StationSystem &cpy)
      : segments(cpy.segments),
        input_segment_index(cpy.input_segment_index),
        platform_alight_segment_mapping(cpy.platform_alight_segment_mapping),
        platform_board_segment_mapping(cpy.platform_board_segment_mapping),
        entrance_flow_rate_indexes(cpy.entrance_flow_rate_indexes),

        split_ratios_path(cpy.split_ratios_path),
        split_ratios(cpy.split_ratios_path),
        cached_split(cpy.cached_split),
        cached_split_slice(cpy.cached_split_slice),

        input_driver(cpy.input_driver, this) {
    // this->input_driver = StationSystemInput(cpy.input_driver, this);
  }
  ~StationSystem() {
    if (split_ratios.is_open())
      split_ratios.close();
  }

  void operator()(const ODE_Solver::Vector &x, ODE_Solver::Vector &dxdt, const double /* t */);
  void _check();

  // Return the state update vector which should be added to the current state
  ODE_Solver::Vector EntranceUpdateVector(std::vector<double> n_people);
  ODE_Solver::Vector PlatformUpdateVector(double n_people, unsigned int platform_id);
  int QueryPlatformDepartingIndex(unsigned int platform_id);
  StationSystemInput InputDriver() { return this->input_driver; }

  unsigned int platform_count() { return platform_board_segment_mapping.size(); }
  std::vector<int> GetOutflowSegments();

  int entrance_count() { return this->input_segment_index.size(); }
  int entrance_flow_index(int i) {
    if (i >= this->entrance_flow_rate_indexes.size())
      return -1;
    else
      return this->entrance_flow_rate_indexes[i];
  }

  private:
  // Standard pedestrian parameters
  constexpr static double v0 = 1.3;
  constexpr static double p_max = 4;
  constexpr static double t_gap = 0.5;

  constexpr static double l_eff = 1 / p_max;
  constexpr static double p_cap = 1 / (v0 * t_gap + l_eff);

  constexpr static int TIME_SERIES_ENTRY_COUNT =
    ((24 * 60) / 15) * 7;                          // 15 minute slices across a day, 7 days
  constexpr static int TIME_SERIES_ENTRY_LEN = 9;  // 9 characters

  StationSystemInput input_driver;
  std::vector<int> input_segment_index;

  std::vector<int> platform_board_segment_mapping;   // Platform segments that you board from
  std::vector<int> platform_alight_segment_mapping;  // Platform segments that you alight onto

  std::vector<int> entrance_flow_rate_indexes;

  std::string split_ratios_path;
  std::basic_ifstream<char> split_ratios;
  std::vector<double> cached_split;
  std::vector<int> cached_split_slice;

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
  static double dQe(double p1_in, double p2_full, double p2_in, double p3_full) {
    return fmin(Qb_out(p1_in), Qb_in(p2_full)) - fmin(Qb_out(p2_in), Qb_in(p3_full));
  }

  inline double _density_factor(int i, double rel_to = 1) {
    return (segments[i].type == AREA_INFLOW || segments[i].type == AREA_OUTFLOW)
             ? segments[i].xk * rel_to
             : 1;
  }
  inline double _split_ratio(int i, double t) {
    if (segments[i].split_ratio != -1)
      return segments[i].split_ratio;

    if (segments[i].split_ratio_series_index == -1)
      throw std::runtime_error("No valid split ratio found");
    //           (minutes % minutes in a day) / entry count per day
    extern double time_offset;
    int slice = ((int)floor(t + time_offset) % (24 * 60)) / 15;

    // Use cached value rather than reading from file again
    if (this->cached_split_slice[i] == slice)
      return this->cached_split[i];

    int foffset =
      (segments[i].split_ratio_series_index * (TIME_SERIES_ENTRY_COUNT * TIME_SERIES_ENTRY_LEN)) +
      (slice * TIME_SERIES_ENTRY_LEN);

    this->split_ratios.seekg(foffset, std::ios_base::beg);
    this->split_ratios.clear();
    char read[9] = "\0\0\0\0\0\0\0\0";
    this->split_ratios.read(read, 8);
    double out = atof(read);
    if (out > 1 || out < 0)
      std::cout << "WARN - read at " << segments[i].split_ratio_series_index << " " << t << "("
                << slice << " " << foffset << ")" << " returned invalid " << read << std::endl;

    this->cached_split_slice[i] = slice;
    this->cached_split[i] = out;
    return out;
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

  struct SegmentData {
    SegmentType type;
    unsigned int prev, next;
    int adjacent = -1;
    double xk;

    // SPLIT_OUTPUT fields
    // These then get read back by SPLIT_INPUT structures in calculations, to avoid data duplication
    unsigned int secondary;
    double split_ratio = -1;
    int split_ratio_series_index = -1;

    SegmentData();
    SegmentData(boost::json::object data);
  };
  std::vector<SegmentData> segments;
};

#endif