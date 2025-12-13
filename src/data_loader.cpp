#include "data_loader.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

SimulationData::SimulationData(const std::string& od_matrix_path,
                               const std::string& boarders_path,
                               const std::string& interchange_path,
                               const std::vector<std::string>& station_names) {
  for (size_t i = 0; i < station_names.size(); ++i) {
    station_name_to_index[station_names[i]] = i;
    station_index_to_name.push_back(station_names[i]);
  }

  load_od_matrix(od_matrix_path);
  load_csv_data(boarders_path, boarders_data);
  load_csv_data(interchange_path, interchange_data);
}

void SimulationData::load_od_matrix(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open OD matrix file: " + path);
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  file.close();

  try {
    od_matrix_json = boost::json::parse(content);
  } catch (const std::exception& e) {
    throw std::runtime_error("Failed to parse OD matrix JSON: " + std::string(e.what()));
  }

  std::cout << "Loaded OD matrix from " << path << std::endl;
}

void SimulationData::load_csv_data(const std::string& path, std::vector<TimeSeriesData>& output) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open CSV file: " + path);
  }

  std::string line;
  std::getline(file, line);

  while (std::getline(file, line)) {
    if (line.empty()) continue;

    std::stringstream ss(line);
    std::string token;
    TimeSeriesData data;
    int col = 0;

    while (std::getline(ss, token, ',')) {
      if (col == 0) {
        data.station = token;
      } else if (col == 1) {
        data.direction = token;
      } else if (col == 2) {
        data.day = token;
      } else if (col == 3) {
        data.total = std::stod(token);
      } else {
        data.time_bins.push_back(std::stod(token));
      }
      col++;
    }

    output.push_back(data);
  }

  file.close();
  std::cout << "Loaded " << output.size() << " records from " << path << std::endl;
}

std::map<int, double> SimulationData::get_od_probabilities(int origin_index,
                                                           const std::string& direction,
                                                           const std::string& day) const {
  std::map<int, double> result;

  if (origin_index < 0 || origin_index >= static_cast<int>(station_index_to_name.size())) {
    return result;
  }

  std::string origin_name = station_index_to_name[origin_index];

  try {
    auto& direction_obj = od_matrix_json.as_object().at(direction).as_object();
    auto& day_obj = direction_obj.at(day).as_object();
    auto& origin_obj = day_obj.at(origin_name).as_object();

    for (const auto& pair : origin_obj) {
      std::string dest_name(pair.key());
      double prob = pair.value().as_double();

      auto it = station_name_to_index.find(dest_name);
      if (it != station_name_to_index.end()) {
        result[it->second] = prob;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Warning: Could not find OD data for origin=" << origin_name 
              << " direction=" << direction << " day=" << day << std::endl;
  }

  return result;
}

double SimulationData::get_entrance_flow(int station_index,
                                        const std::string& direction,
                                        const std::string& day,
                                        int time_bin_index) const {
  if (station_index < 0 || station_index >= static_cast<int>(station_index_to_name.size())) {
    return 0.0;
  }

  std::string station_name = station_index_to_name[station_index];

  for (const auto& data : boarders_data) {
    if (data.station == station_name && 
        data.direction == direction && 
        data.day == day) {
      if (time_bin_index >= 0 && time_bin_index < static_cast<int>(data.time_bins.size())) {
        return data.time_bins[time_bin_index];
      }
    }
  }

  return 0.0;
}

double SimulationData::get_interchange_flow(int station_index,
                                           const std::string& direction,
                                           const std::string& day,
                                           int time_bin_index) const {
  if (station_index < 0 || station_index >= static_cast<int>(station_index_to_name.size())) {
    return 0.0;
  }

  std::string station_name = station_index_to_name[station_index];

  for (const auto& data : interchange_data) {
    if (data.station == station_name && 
        data.direction == direction && 
        data.day == day) {
      if (time_bin_index >= 0 && time_bin_index < static_cast<int>(data.time_bins.size())) {
        return data.time_bins[time_bin_index];
      }
    }
  }

  return 0.0;
}

int SimulationData::get_station_index(const std::string& name) const {
  auto it = station_name_to_index.find(name);
  if (it != station_name_to_index.end()) {
    return it->second;
  }
  return -1;
}

std::string SimulationData::get_station_name(int index) const {
  if (index >= 0 && index < static_cast<int>(station_index_to_name.size())) {
    return station_index_to_name[index];
  }
  return "";
}