#include "station_ode.hpp"
#include "ode/json_util.hpp"

void StationSystemInput::operator()(ODE_Solver::Vector &x, double t) {
  if (this->system == NULL)
    return;

  // Const input (used in testing)
  if (input_n >= 0) {
    x += system->EntranceUpdateVector(std::vector<double>(1, input_n));
    return;
  }
  // RVG-driven input
  //  ...
}

StationSystem::StationSystem(boost::json::object data) {
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
  this->initial_state = ODE_Solver::Vector(len);
  if (!data.at("initial_state").is_array())
    throw std::runtime_error(
      "Invalid JSON value for station data, expected array for key [initial_state]");
  auto state = data.at("initial_state").as_array();
  if (state.size() != len)
    throw std::runtime_error("Invalid JSON value for station data, expected array [initial_state] "
                             "to have equal length to [structure]");

  for (int i = 0; i < len; ++i) {
    if (!state.at(i).is_number()) {
      throw std::runtime_error(
        "Invalid JSON value for station data, expected numeric inside array [initial_state]");
    }
    JSON_ParseNumericToDouble(this->initial_state[i], &state.at(i));
  }

  // Setup input driver (optional, defaults to const 0 input)
  if (data.contains("input")) {
    this->input_driver.system = this;
    if (data.at("input").is_number())
      JSON_ParseNumericToDouble(this->input_driver.input_n, &data.at("input"));
    else {
    }
  }
}

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

  if (!data.contains("linked_to_area"))
    this->linked_to_area = NONE;
  else if (!data.at("linked_to_area").is_string())
    throw std::runtime_error(
      "Invalid JSON value for station segment data, missing/mistyped key [linked_to_area]");
  else {
    str = data.at("linked_to_area").as_string();
    if (str == "NONE")
      this->linked_to_area = AreaLink::NONE;
    else if (str == "FROM")
      this->linked_to_area = AreaLink::FROM;
    else if (str == "TO")
      this->linked_to_area = AreaLink::TO;
    else if (str == "BOTH")
      this->linked_to_area = AreaLink::BOTH;
  }

  if (this->type == SPLIT_OUTPUT) {  // Read extra fields
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

    if (!data.contains("split_ratio") || (!data.at("split_ratio").is_number()))
      throw std::runtime_error(
        "Invalid JSON value for station segment data, missing/mistyped key [split_ratio]");
    val = data.at("split_ratio");
    JSON_ParseNumericToDouble(this->split_ratio, &val);
  }
}

void StationSystem::operator()(const ODE_Solver::Vector &x, ODE_Solver::Vector &dxdt,
                               const double /* t */) {
  for (int i = 0; i < x.size(); ++i) {
    double in_density_factor = _in_density_factor(i);
    double out_density_factor = _out_density_factor(i);

    double xk = segments[i].xk;
    if (segments[i].adjacent != -1)
      xk += segments[segments[i].adjacent].xk;
    double fct = 1 / xk;

    switch (segments[i].type) {
      case SegmentType::INVALID:
        throw std::runtime_error("Uninitialised segment in station system");
      case SegmentType::DIRECT: {
        double p_inflow = x[segments[i].prev] / in_density_factor;
        double p_outflow = x[segments[i].next] / out_density_factor;
        double p_self = x[i];
        double p_self_full = p_self;

        if (segments[segments[i].prev].adjacent != -1)
          p_inflow /= 2;
        if (segments[segments[i].next].adjacent != -1)
          // Look opposite direction
          p_outflow = (p_outflow + x[segments[segments[i].adjacent].prev] /
                                     _in_density_factor(segments[i].adjacent)) /
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
        double p_inflow = x[segments[i].prev] / in_density_factor;

        if (segments[i].adjacent != -1) {
          p_self = (p_self + x[segments[i].adjacent] / segments[segments[i].adjacent].xk) / 2;
        }
        if (segments[segments[i].prev].adjacent != -1)
          p_inflow /= 2;

        dxdt[i] = fmin(Qb_out(p_inflow), Qb_in(p_self));
        break;
      }
      case SegmentType::AREA_OUTFLOW: {
        double p_outflow = x[segments[i].next] / out_density_factor;
        double p_self = x[i] / segments[i].xk;

        if (segments[segments[i].next].adjacent != -1) {
          p_outflow = (p_outflow + x[segments[segments[i].next].adjacent] /
                                     segments[segments[segments[i].next].adjacent].xk) /
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
        double p_inflow = x[segments[i].prev] / in_density_factor;
        double p_outflow1 = x[segments[i].next];  //  / out_density_factor
        double p_outflow2 = x[segments[i].secondary];
        double p_self = x[i];
        double p_self_full = p_self;

        if (segments[segments[i].prev].adjacent != -1)
          p_inflow /= 2;
        if (segments[segments[i].next].adjacent != -1)
          p_outflow1 = (p_outflow1 + x[segments[segments[i].next].adjacent]) / 2;
        if (segments[segments[i].secondary].adjacent != -1)
          p_outflow2 = (p_outflow2 + x[segments[segments[i].secondary].adjacent]) / 2;
        if (segments[i].adjacent != -1) {
          p_self_full = (p_self_full + x[segments[i].adjacent]) / 2;
          p_self /= 2;
        }

        double p_outflow = fmax(p_outflow1, p_outflow2);

        dxdt[i] = fct * (dQe(p_inflow, p_self_full, p_self * segments[i].split_ratio, p_outflow) -
                         fmin(Qb_out(p_self * (1 - segments[i].split_ratio)), Qb_in(p_outflow)));
        break;
      }
      case SegmentType::SPLIT_INPUT: {
        double p_inflow1 = x[segments[i].prev] / in_density_factor;
        double p_inflow2 = x[segments[i].secondary];
        double p_outflow = x[segments[i].next];
        double p_self = x[i];
        double p_self_full = p_self;

        double split_ratio1, split_ratio2;
        double p_alternative1 = 0, p_alternative2 = 0;

        if (segments[segments[i].prev].next == i) {
          split_ratio1 = segments[segments[i].prev].split_ratio;

          int alt = segments[segments[i].prev].secondary;
          p_alternative1 = x[alt];
          if (segments[alt].adjacent != -1) {
            p_alternative1 = (p_alternative1 + x[segments[alt].adjacent]) / 2;
          }
        }
        else {
          split_ratio1 = 1 - segments[segments[i].prev].split_ratio;

          int alt = segments[segments[i].prev].next;
          p_alternative1 = x[alt];
          if (segments[alt].adjacent != -1) {
            p_alternative1 = (p_alternative1 + x[segments[alt].adjacent]) / 2;
          }
        }
        if (segments[segments[i].secondary].next == i) {
          split_ratio2 = segments[segments[i].secondary].split_ratio;

          int alt = segments[segments[i].secondary].secondary;
          p_alternative2 = x[alt];
          if (segments[alt].adjacent != -1) {
            p_alternative2 = (p_alternative2 + x[segments[alt].adjacent]) / 2;
          }
        }
        else {
          split_ratio2 = 1 - segments[segments[i].secondary].split_ratio;

          int alt = segments[segments[i].secondary].next;
          p_alternative2 = x[alt];
          if (segments[alt].adjacent != -1) {
            p_alternative2 = (p_alternative2 + x[segments[alt].adjacent]) / 2;
          }
        }

        if (segments[segments[i].prev].adjacent != -1)
          p_inflow1 /= 2;
        if (segments[segments[i].secondary].adjacent != -1)
          p_inflow2 /= 2;
        if (segments[segments[i].next].adjacent != -1)
          p_outflow = (p_outflow + x[segments[segments[i].next].adjacent]) / 2;
        if (segments[i].adjacent != -1) {
          p_self_full = (p_self_full + x[segments[i].adjacent]) / 2;
          p_self /= 2;
        }

        p_alternative1 = fmax(p_self_full, p_alternative1);
        p_alternative2 = fmax(p_self_full, p_alternative2);

        dxdt[i] = fct * (dQe(p_inflow1 * split_ratio1, p_alternative1, p_self, p_outflow) +
                         fmin(Qb_out(p_inflow2 * split_ratio2), Qb_out(p_alternative2)));
        break;
      }
    }
  }
}