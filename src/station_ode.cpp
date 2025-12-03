#include "station_ode.hpp"
#include "ode/json_util.hpp"

StationSystem::StationSystem(boost::json::array structure) {
  unsigned long len = structure.size();
  segments = std::vector<SegmentData>(len);

  for (int i = 0; i < len; ++i) {
    if (!structure.at(i).is_object()) {
      printf("WARN - invalid station structure field");
      break;
    }
    segments[i] = SegmentData(structure.at(i).as_object());
  }
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
    double in_density_factor =
      segments[i].linked_to_area & FROM ? segments[segments[i].prev].xk : 1;
    double out_density_factor = segments[i].linked_to_area & TO ? segments[segments[i].next].xk : 1;
    double fct = 1 / segments[i].xk;

    switch (segments[i].type) {
      case SegmentType::INVALID:
        throw std::runtime_error("Uninitialised segment in station system");
      case SegmentType::DIRECT:
        dxdt[i] = fct * dQe(x[segments[i].prev] / in_density_factor, x[i],
                            x[segments[i].next] / out_density_factor);
        break;
      case SegmentType::AREA_INFLOW:
        dxdt[i] =
          fmin(Qb_out(x[segments[i].prev] / in_density_factor), Qb_in(x[i] / segments[i].xk));
        break;
      case SegmentType::AREA_OUTFLOW:
        dxdt[i] =
          -fmin(Qb_out(x[i] / segments[i].xk), Qb_in(x[segments[i].next] / out_density_factor));
        break;
      case SegmentType::SPLIT_OUTPUT:
        dxdt[i] =
          fct *
          dQe(x[segments[i].prev] / in_density_factor, x[i],
              (x[segments[i].next] * segments[i].split_ratio / out_density_factor) +
                (x[segments[i].secondary] * (1 - segments[i].split_ratio) / out_density_factor));
        break;
      case SegmentType::SPLIT_INPUT:
        double split_ratio = segments[segments[i].prev].next == i
                               ? segments[segments[i].prev].split_ratio
                               : 1 - segments[segments[i].prev].split_ratio;
        dxdt[i] = fct * dQe_split_in(x[segments[i].prev] / in_density_factor, x[i],
                                     x[segments[i].next] / out_density_factor, split_ratio);
        break;
    }
  }
}