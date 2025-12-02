#include "station_ode.hpp"
#include "ode/json_util.hpp"

StationSystem::StationSystem(boost::json::array structure) {
  segments = std::vector<SegmentData>(structure.size());

  int i = 0;
  for (auto it = structure.begin(); it != structure.end(); ++it, ++i) {
    if (!it->is_object()) {
      printf("WARN - invalid station structure field");
      break;
    }
    segments[i] = SegmentData(it->as_object());
  }
}
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

  if (!data.contains("next") || (!data.at("next").is_int64() && !data.at("next").is_uint64()))
    throw std::runtime_error(
      "Invalid JSON value for station segment data, missing/mistyped key [next]");
  val = data.at("next");
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

  if (!data.contains("xk") || (!data.at("xk").is_number()))
    throw std::runtime_error(
      "Invalid JSON value for station segment data, missing/mistyped key [xk]");
  val = data.at("xk");
  JSON_ParseNumericToDouble(this->xk, &val);

  if (!data.contains("from_area") || (!data.at("from_area").is_bool()))
    throw std::runtime_error(
      "Invalid JSON value for station segment data, missing/mistyped key [from_area]");
  this->from_area = data.at("from_area").as_bool();

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
    double in_density_factor = segments[i].from_area ? segments[i].xk : 1;
    double fct = 1 / segments[i].xk;

    switch (segments[i].type) {
      case SegmentType::DIRECT:
        dxdt[i] = fct * dQe(x[segments[i].prev] / in_density_factor, x[i], x[segments[i].next]);
        break;
      case SegmentType::AREA_INFLOW:
        dxdt[i] = Qb_out(x[segments[i].prev]);
        break;
      case SegmentType::AREA_OUTFLOW:
        dxdt[i] = -fmin(Qb_out(x[i] / in_density_factor), Qb_in(x[segments[i].next]));
        break;
      case SegmentType::SPLIT_OUTPUT:
        dxdt[i] = fct * dQe(x[segments[i].prev] / in_density_factor, x[i],
                            (x[segments[i].next] * segments[i].split_ratio) +
                              (x[segments[i].secondary] * (1 - segments[i].split_ratio)));
        break;
      case SegmentType::SPLIT_INPUT:
        double split_ratio = segments[segments[i].prev].next == i
                               ? segments[segments[i].prev].split_ratio
                               : 1 - segments[segments[i].prev].split_ratio;
        dxdt[i] = fct * dQe_split_in(x[segments[i].prev] / in_density_factor, x[i],
                                     x[segments[i].next], split_ratio);
        break;
    }
  }
}