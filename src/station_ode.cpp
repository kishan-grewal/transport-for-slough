#include "station_ode.hpp"
#include "ode/json_util.hpp"

//! StationSystemInput constructor
/*!
Create a station system input
\param system StationSystem* pointer to the StationSystem that is linked to this input
\param input int population added by the input

For the input, provide a StationSystem to be linked to
Set the number of inputs to the StationSystem
Set the timestamp of the previous update
Allocate the target_inputs, accumulated inputs and interp_t vectors with the number of entrances to the station system
*/
StationSystemInput::StationSystemInput(StationSystem *system, int input)
    : system(system),
      input_n(input),
      last_update_t(0),
      target_inputs(system == NULL ? 0 : system->entrance_count(), 0),
      accumulated(system == NULL ? 0 : system->entrance_count(), 0),
      interp_t(system == NULL ? 0 : system->entrance_count(), 0) {}


//! Update the input for the timestep
/*!
Update the number of people inflow.
\param x ODE_Solver::Vector& the current populations for the segments of the station
\param t double the timestamp of the update

Add constant population on all segments if constant input specified
Else if enough time elapsed update targets, add input flow and subtract the accumulated
Otherwise calculate delta input interpolated from time multiplied by inputs, then subtracting accumulated populations
Then adding the delta to the accumulated population and returning the accumulated input populations
Finally return the population and update to new timestep
*/
void StationSystemInput::operator()(ODE_Solver::Vector &x, double t) {
  if (this->system == NULL)
    return;

  // Const input (used in testing)
  if (input_n >= 0) {
    x += system->EntranceUpdateVector(std::vector<double>(system->entrance_count(), input_n));
    return;
  }
  // RVG-driven input
  //  ...
  int flow_index;
  auto update_vector = std::vector<double>(system->entrance_count(), 0);
  for (int i = 1; i < system->entrance_count(); ++i) {
    // Update flow target if enough time has passed
    if (t >= this->last_update_t + TIME_SERIES_TIMESTEP) {
      flow_index = system->entrance_flow_index(i);
      if (flow_index == -1) {
        std::cout
          << "WARN - misconfigured input parameters, unable to get input flow index for entrance "
          << i << std::endl;
      }

      else {
        int slice = ((int)floor(t / 60) % (24 * 60)) / TIME_SERIES_TIMESTEP;
        int foffset = (flow_index * (TIME_SERIES_ENTRY_COUNT * TIME_SERIES_ENTRY_LEN)) +
                      (slice * TIME_SERIES_ENTRY_LEN);

        inflows->seekg(foffset, std::ios_base::beg);
        char read[9] = "\0\0\0\0\0\0\0\0";
        inflows->read(read, 8);
        this->target_inputs[i] += atof(read);

        this->target_inputs[i] -= this->accumulated[i];
        this->accumulated[i] = 0;
      }
    }

    //
    double interp = this->target_inputs[i] * (fmod(t, TIME_SERIES_TIMESTEP) / TIME_SERIES_TIMESTEP);
    double delta = interp - this->accumulated[i];
    if (delta < 1)
      continue;
    delta = floor(delta);
    this->accumulated[i] += delta;
    update_vector[i] += delta;
  }
  x += system->EntranceUpdateVector(update_vector);

  // Do this at the end, not in the loop
  if (t >= this->last_update_t + TIME_SERIES_TIMESTEP)
    this->last_update_t += TIME_SERIES_TIMESTEP;  // Update
}

//! StationSystem constructor
/*!
Parse JSON for station topology
\param data boost::json::object contains topology data for the station
\param split_ratios std::ifstream& not sure
\param input_timestep double time between updating inputs

Check JSON is valid before unwrapping.
Initialise the segments with SegmentData objects
Add indexes of entrances to input_segment_index
Get platform IDs
Check segment type, if INFLOW or OUTFLOW
Resize board/alight_segment_mappings to fit the segments as they get loaded
Add segment index to board/alight index mappings
Set initial state and input driver (input driver generates inflow/outflow from entrances)
Set input driver timestep
*/
StationSystem::StationSystem(boost::json::object data, std::ifstream &split_ratios,
                             double input_timestep)
    : split_ratios(split_ratios) {
  // Setup internal equation structure
  if (!data.at("structure").is_array())
    throw std::runtime_error(
      "Invalid JSON value for station data, expected array for key [structure]");
  auto structure = data.at("structure").as_array();
  unsigned long len = structure.size();
  this->segments = std::vector<SegmentData>(len);

  for (int i = 0; i < len; ++i) {
    if (!structure.at(i).is_object()) {
      printf("WARN - invalid station structure field");
      break;
    }
    auto segment = structure.at(i).as_object();
    this->segments[i] = SegmentData(segment);

    if (segment.contains("is_entrance") && segment.at("is_entrance").is_bool() &&
        segment.at("is_entrance").as_bool()) {
      this->input_segment_index.push_back(i);
    }
    else if (segment.contains("platform_id")) {
      int id;
      if (segment.at("platform_id").is_int64())
        id = segment.at("platform_id").as_int64();
      else if (segment.at("platform_id").is_uint64())
        id = segment.at("platform_id").as_uint64();
      else
        throw std::runtime_error(
          "Invalid JSON value for station data, expected integer for key [platform_id]");

      switch (this->segments[i].type) {
        case AREA_INFLOW:
          if (id >= this->platform_board_segment_mapping.size())
            this->platform_board_segment_mapping.resize(id + 1);
          this->platform_board_segment_mapping[id] = i;
          break;
        case AREA_OUTFLOW:
          if (id >= this->platform_alight_segment_mapping.size())
            this->platform_alight_segment_mapping.resize(id + 1);
          this->platform_alight_segment_mapping[id] = i;
          break;
        default:
          throw std::runtime_error("Non-area segment cannot be specified as a platform");
      }
    }
  }

  // Setup initial state
  //
  // Array initial state
  this->initial_state = ODE_Solver::Vector(len);
  if (data.at("initial_state").is_array()) {
    auto state = data.at("initial_state").as_array();
    if (state.size() != len)
      throw std::runtime_error(
        "Invalid JSON value for station data, expected array [initial_state] "
        "to have equal length to [structure]");

    for (int i = 0; i < len; ++i) {
      if (!state.at(i).is_number()) {
        throw std::runtime_error(
          "Invalid JSON value for station data, expected numeric inside array [initial_state]");
      }
      JSON_ParseNumericToDouble(this->initial_state[i], &state.at(i));
    }
  }
  // Constant initial state
  else if (data.at("initial_state").is_number()) {
    double val = 0;
    JSON_ParseNumericToDouble(val, &data.at("initial_state"));
    this->initial_state = ODE_Solver::Vector(len, val);
  }
  else
    throw std::runtime_error(
      "Invalid JSON value for station data, expected array for key [initial_state]");

  // Setup input driver (optional, defaults to const 0 input)
  if (data.contains("input")) {
    this->input_driver.system = this;
    if (data.at("input").is_number())
      JSON_ParseNumericToDouble(this->input_driver.input_n, &data.at("input"));
    else {
    }
  }
  this->input_driver.timestep = input_timestep;
}

//! Update populations from entrances in station
/*!
For each segment add the correlated number of people in each input segment
*/
ODE_Solver::Vector StationSystem::EntranceUpdateVector(std::vector<double> n_people) {
  if (this->input_segment_index.size() != n_people.size()) {
    throw std::runtime_error("Invalid input size for station system");
  }

  ODE_Solver::Vector out = ODE_Solver::Vector(this->segments.size(), 0);
  for (int i = 0; i < n_people.size(); ++i) {
    out[this->input_segment_index[i]] += n_people[i];
  }
  return out;
}

//! Gives the vector for updating platform population from discrete events
/*!
Returns platform update vector for train boarding
\param n_people double number of people alighting (positive) or boarding (negative)
\param platform_id unsigned int platform ID so only segments on that platform get inflow/outflow
*/
ODE_Solver::Vector StationSystem::PlatformUpdateVector(double n_people, unsigned int platform_id) {
  ODE_Solver::Vector out = ODE_Solver::Vector(this->segments.size(), 0);
  if (n_people > 0) {  // People alighting from a train
    if (platform_id >= platform_alight_segment_mapping.size())
      throw std::runtime_error("Out of bounds platform id for station system");
    out[this->platform_alight_segment_mapping[platform_id]] += n_people;
  }
  else if (n_people < 0) {  // People boarding from a platform
    if (platform_id >= platform_board_segment_mapping.size())
      throw std::runtime_error("Out of bounds platform id for station system");
    out[this->platform_board_segment_mapping[platform_id]] += n_people;
  }
  return out;
}

StationSystem::SegmentData::SegmentData() { this->type = SegmentType::INVALID; }

//!
StationSystem::SegmentData::SegmentData(boost::json::object data) {
  if (!data.contains("type") || !data.at("type").is_string())
    throw std::runtime_error(
      "Invalid JSON value for station segment data, missing/mistyped key [type]");
  auto str = data.at("type").as_string();
  if (str == "DIRECT")
    this->type = SegmentType::DIRECT;
  else if (str == "SPLIT_INPUT")
    this->type = SegmentType::SPLIT_INPUT;
  else if (str == "SPLIT_OUTPUT")
    this->type = SegmentType::SPLIT_OUTPUT;
  else if (str == "AREA_INFLOW")
    this->type = SegmentType::AREA_INFLOW;
  else if (str == "AREA_OUTFLOW")
    this->type = SegmentType::AREA_OUTFLOW;
  else
    throw std::runtime_error(
      "Invalid JSON value for station segment data, invalid value at key [type]");

  if (this->type != AREA_OUTFLOW) {
    if (!data.contains("prev") || (!data.at("prev").is_int64() && !data.at("prev").is_uint64()))
      throw std::runtime_error(
        "Invalid JSON value for station segment data, missing/mistyped key [prev]");

    auto &val = data.at("prev");
    switch (val.kind()) {
      case boost::json::kind::int64:
        this->prev = val.as_int64();
        break;
      case boost::json::kind::uint64:
        this->prev = val.as_uint64();
        break;
      default:  // Should be caught in above statement
        break;
    }
  }

  if (this->type != AREA_INFLOW) {
    if (!data.contains("next") || (!data.at("next").is_int64() && !data.at("next").is_uint64()))
      throw std::runtime_error(
        "Invalid JSON value for station segment data, missing/mistyped key [next]");

    auto &val = data.at("next");
    switch (val.kind()) {
      case boost::json::kind::int64:
        this->next = val.as_int64();
        break;
      case boost::json::kind::uint64:
        this->next = val.as_uint64();
        break;
      default:  // Should be caught in above statement
        break;
    }
  }

  if (data.contains("adjacent")) {
    auto &val = data.at("adjacent");
    switch (val.kind()) {
      case boost::json::kind::int64:
        this->adjacent = val.as_int64();
        break;
      case boost::json::kind::uint64:
        this->adjacent = val.as_uint64();
        break;
      default:  // Should be caught in above statement
        throw std::runtime_error("Type mismatch corruption");
    }
  }
  else
    this->adjacent = -1;

  if (!data.contains("xk") || (!data.at("xk").is_number()))
    throw std::runtime_error(
      "Invalid JSON value for station segment data, missing/mistyped key [xk]");
  auto &val = data.at("xk");
  JSON_ParseNumericToDouble(this->xk, &val);

  if (this->type == SPLIT_OUTPUT || this->type == SPLIT_INPUT) {  // Read extra fields
    if (!data.contains("secondary") ||
        (!data.at("secondary").is_int64() && !data.at("secondary").is_uint64()))
      throw std::runtime_error(
        "Invalid JSON value for station segment data, missing/mistyped key [secondary]");
    val = data.at("secondary");
    switch (val.kind()) {
      case boost::json::kind::int64:
        this->secondary = val.as_int64();
        break;
      case boost::json::kind::uint64:
        this->secondary = val.as_uint64();
        break;
      default:  // Should be caught in above statement
        break;
    }
  }

  if (this->type == SPLIT_OUTPUT) {
    if (!(data.contains("split_ratio")))
      throw std::runtime_error(
        "Invalid JSON value for station segment data, missing/mistyped key [split_ratio]");

    if (data.at("split_ratio").is_number()) {
      val = data.at("split_ratio");
      JSON_ParseNumericToDouble(this->split_ratio, &val);
      // Bounds check, and also check for failure in generation script (initialises them to -1,
      // before calculating ratios)
      if (this->split_ratio < 0 || this->split_ratio > 1) {
        std::cout << this->split_ratio << std::endl;
        throw std::runtime_error("Invalid JSON value for station segment data, value at key "
                                 "[split_ratio] must be >= 0 and <= 1");
      }
    }
    else if (data.at("split_ratio").is_object()) {
      // throw std::runtime_error("Time-series driven split ratio not implemented yet");
      auto &ratio = data.at("split_ratio").as_object();
      if (!(ratio.contains("index") &&
            (ratio.at("index").is_int64() || ratio.at("index").is_uint64())))
        throw std::runtime_error(
          "Invalid JSON value for station segment data, missing/mistyped  key [split_ratio.index]");

      if (ratio.at("index").is_int64())
        this->split_ratio_series_index = ratio.at("index").as_int64();
      else if (ratio.at("index").is_uint64())
        this->split_ratio_series_index = ratio.at("index").as_uint64();
    }
  }
}

void StationSystem::operator()(const ODE_Solver::Vector &x, ODE_Solver::Vector &dxdt,
                               const double t) {
  for (int i = 0; i < x.size(); ++i) {
    // double in_density_factor = _in_density_factor(i);
    // double out_density_factor = _out_density_factor(i);

    double xk = segments[i].xk;
    if (segments[i].adjacent != -1)
      xk += segments[segments[i].adjacent].xk;
    double fct = 1 / xk;

    switch (segments[i].type) {
      case SegmentType::INVALID:
        throw std::runtime_error("Uninitialised segment in station system");
      case SegmentType::DIRECT: {
        double p_inflow = x[segments[i].prev] / _density_factor(segments[i].prev);
        double p_outflow = x[segments[i].next] / _density_factor(segments[i].next);
        double p_self = x[i];
        double p_self_full = p_self;

        if (segments[segments[i].prev].adjacent != -1)
          p_inflow /= 2;
        if (segments[segments[i].next].adjacent != -1)
          // Look opposite direction
          p_outflow = (p_outflow + x[segments[segments[i].adjacent].prev] /
                                     _density_factor(segments[segments[i].adjacent].prev)) /
                      2;
        if (segments[i].adjacent != -1) {
          p_self_full = (p_self_full + x[segments[i].adjacent]) / 2;
          p_self /= 2;
        }

        dxdt[i] = fct * dQe(p_inflow, p_self_full, p_self, p_outflow);
        break;
      }
      case SegmentType::AREA_INFLOW: {
        double p_self = x[i] / segments[i].xk;
        double p_inflow = x[segments[i].prev] / _density_factor(segments[i].prev);

        if (segments[i].adjacent != -1) {
          p_self = (p_self + x[segments[i].adjacent] / _density_factor(segments[i].adjacent)) / 2;
        }
        if (segments[segments[i].prev].adjacent != -1)
          p_inflow /= 2;

        dxdt[i] = fmin(Qb_out(p_inflow), Qb_in(p_self));
        break;
      }
      case SegmentType::AREA_OUTFLOW: {
        double p_outflow = x[segments[i].next] / _density_factor(segments[i].next);
        double p_self = x[i] / segments[i].xk;

        if (segments[segments[i].next].adjacent != -1) {
          p_outflow = (p_outflow + x[segments[segments[i].next].adjacent] /
                                     _density_factor(segments[segments[i].next].adjacent)) /
                      2;
        }
        if (segments[i].adjacent != -1) {
          p_self /= 2;
        }

        // x[i] / segments[i].xk / 2
        dxdt[i] = -fmin(Qb_out(p_self), Qb_in(p_outflow));
        break;
      }
      case SegmentType::SPLIT_OUTPUT: {
        double p_inflow = x[segments[i].prev] / _density_factor(segments[i].prev);
        double p_outflow1 = x[segments[i].next] / _density_factor(segments[i].next);
        double p_outflow2 = x[segments[i].secondary] / _density_factor(segments[i].secondary);
        double p_self = x[i];
        double p_self_full = p_self;

        if (segments[segments[i].prev].adjacent != -1)
          p_inflow /= 2;
        if (segments[segments[i].next].adjacent != -1)
          p_outflow1 = (p_outflow1 + x[segments[segments[i].next].adjacent] /
                                       _density_factor(segments[segments[i].next].adjacent)) /
                       2;
        if (segments[segments[i].secondary].adjacent != -1)
          p_outflow2 = (p_outflow2 + x[segments[segments[i].secondary].adjacent] /
                                       _density_factor(segments[segments[i].secondary].adjacent)) /
                       2;
        if (segments[i].adjacent != -1) {
          p_self_full =
            (p_self_full + x[segments[i].adjacent] / _density_factor(segments[i].adjacent)) / 2;
          p_self /= 2;
        }

        double p_outflow = fmax(p_outflow1, p_outflow2);

        double split_ratio = _split_ratio(i, t);
        dxdt[i] = fct * (dQe(p_inflow, p_self_full, p_self * split_ratio, p_outflow) -
                         fmin(Qb_out(p_self * (1 - split_ratio)), Qb_in(p_outflow)));
        break;
      }
      case SegmentType::SPLIT_INPUT: {
        double p_inflow1 = x[segments[i].prev] / _density_factor(segments[i].prev);
        double p_inflow2 = x[segments[i].secondary] / _density_factor(segments[i].secondary);
        double p_outflow = x[segments[i].next] / _density_factor(segments[i].next);
        double p_self = x[i];
        double p_self_full = p_self;

        double split_ratio1 = _split_ratio(segments[i].prev, t),
               split_ratio2 = _split_ratio(segments[i].secondary, t);
        double p_alternative1 = 0, p_alternative2 = 0;

        if (segments[segments[i].prev].next == i) {
          int alt = segments[segments[i].prev].secondary;
          p_alternative1 = x[alt] / _density_factor(alt);
          if (segments[alt].adjacent != -1) {
            p_alternative1 = (p_alternative1 +
                              x[segments[alt].adjacent] / _density_factor(segments[alt].adjacent)) /
                             2;
          }
        }
        else {
          split_ratio1 = 1 - split_ratio1;

          int alt = segments[segments[i].prev].next;
          p_alternative1 = x[alt] / _density_factor(alt);
          if (segments[alt].adjacent != -1) {
            p_alternative1 = (p_alternative1 +
                              x[segments[alt].adjacent] / _density_factor(segments[alt].adjacent)) /
                             2;
          }
        }
        if (segments[segments[i].secondary].next == i) {
          int alt = segments[segments[i].secondary].secondary;
          p_alternative2 = x[alt] / _density_factor(alt);
          if (segments[alt].adjacent != -1) {
            p_alternative2 = (p_alternative2 +
                              x[segments[alt].adjacent] / _density_factor(segments[alt].adjacent)) /
                             2;
          }
        }
        else {
          split_ratio2 = 1 - split_ratio2;

          int alt = segments[segments[i].secondary].next;
          p_alternative2 = x[alt] / _density_factor(alt);
          if (segments[alt].adjacent != -1) {
            p_alternative2 = (p_alternative2 +
                              x[segments[alt].adjacent] / _density_factor(segments[alt].adjacent)) /
                             2;
          }
        }

        if (segments[segments[i].prev].adjacent != -1)
          p_inflow1 /= 2;
        if (segments[segments[i].secondary].adjacent != -1)
          p_inflow2 /= 2;
        if (segments[segments[i].next].adjacent != -1)
          p_outflow = (p_outflow + x[segments[segments[i].next].adjacent] /
                                     _density_factor(segments[segments[i].next].adjacent)) /
                      2;
        if (segments[i].adjacent != -1) {
          p_self_full =
            (p_self_full + x[segments[i].adjacent] / _density_factor(segments[i].adjacent)) / 2;
          p_self /= 2;
        }

        p_alternative1 = fmax(p_self_full, p_alternative1);
        p_alternative2 = fmax(p_self_full, p_alternative2);

        dxdt[i] = fct * (dQe(p_inflow1 * split_ratio1, p_alternative1, p_self, p_outflow) +
                         fmin(Qb_out(p_inflow2 * split_ratio2), Qb_in(p_alternative2)));
        break;
      }
    }
  }
}

void StationSystem::_check() {
  int l = this->segments.size();
  for (int i = 0; i < l; ++i) {
    if (this->segments[i].adjacent >= l || this->segments[i].adjacent < -1)
      std::cout << "Invalid adjacent value for segment " << i << " [" << this->segments[i].adjacent
                << "]" << std::endl;

    if (this->segments[i].type == SegmentType::AREA_OUTFLOW) {
      if (this->segments[i].next >= l || this->segments[i].next < 0)
        std::cout << "Invalid next value for segment " << i << std::endl;
      continue;
    }
    if (this->segments[i].type == SegmentType::AREA_INFLOW) {
      if (this->segments[i].prev >= l || this->segments[i].prev < 0)
        std::cout << "Invalid prev value for segment " << i << std::endl;
      continue;
    }

    if (this->segments[i].prev >= l || this->segments[i].prev < 0 || this->segments[i].next >= l ||
        this->segments[i].next < 0) {
      std::cout << "Invalid prev/next value for segment " << i << std::endl;
      continue;
    }
    if ((this->segments[i].type == SegmentType::SPLIT_OUTPUT ||
         this->segments[i].type == SegmentType::SPLIT_INPUT) &&
        (this->segments[i].secondary >= l || this->segments[i].prev < 0)) {
      std::cout << "Invalid secondary value for segment " << i << std::endl;
      continue;
    }
  }
}