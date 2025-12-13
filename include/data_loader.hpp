#ifndef DATA_LOADER_HPP
#define DATA_LOADER_HPP

#include <boost/json.hpp>
#include <string>
#include <map>
#include <vector>

struct TimeSeriesData {
  std::string station;
  std::string direction;
  std::string day;
  std::vector<double> time_bins;
  double total;
};

class SimulationData {
  private:
  boost::json::value od_matrix_json;
  std::vector<TimeSeriesData> boarders_data;
  std::vector<TimeSeriesData> interchange_data;

  std::map<std::string, int> station_name_to_index;
  std::vector<std::string> station_index_to_name;

  public:
  SimulationData(const std::string& od_matrix_path, 
                 const std::string& boarders_path,
                 const std::string& interchange_path,
                 const std::vector<std::string>& station_names);

  std::map<int, double> get_od_probabilities(int origin_index, 
                                             const std::string& direction,
                                             const std::string& day) const;

  double get_entrance_flow(int station_index,
                          const std::string& direction, 
                          const std::string& day,
                          int time_bin_index) const;

  double get_interchange_flow(int station_index,
                             const std::string& direction,
                             const std::string& day, 
                             int time_bin_index) const;

  int get_station_index(const std::string& name) const;
  std::string get_station_name(int index) const;

  private:
  void load_od_matrix(const std::string& path);
  void load_csv_data(const std::string& path, std::vector<TimeSeriesData>& output);
};

#endif