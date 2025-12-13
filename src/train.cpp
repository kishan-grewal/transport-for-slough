#include "train.hpp"
#include <random>
#include <algorithm>
#include <iostream>

Train::Train(int capacity, int startIndex, int endIndex, int direction, int passenger_count) {
  this->capacity = capacity;
  this->startIndex = startIndex;
  this->endIndex = endIndex;
  this->direction = direction;
  this->passenger_count = passenger_count;
}

int Train::getCapacity() const { return this->capacity; }

int Train::getStartIndex() const { return this->startIndex; }

int Train::getEndIndex() const { return this->endIndex; }

int Train::getDirection() const { return this->direction; }

void Train::setDirection(int newDirection) { this->direction = newDirection; }

int Train::get_passenger_count() const { return this->passenger_count; }

int Train::get_remaining_capacity() const { return this->capacity - this->passenger_count; }

int Train::get_passengers_for_station(int station_index) const {
  auto it = this->passengers_by_destination.find(station_index);
  return (it != this->passengers_by_destination.end()) ? it->second : 0;
}

int Train::alight(int current_station, int max_platform_capacity) {
  int passengers_for_station = get_passengers_for_station(current_station);
  int actual_alight = std::min(passengers_for_station, max_platform_capacity);

  if (actual_alight < passengers_for_station) {
    std::cerr << "WARNING: Platform capacity exceeded - " 
              << (passengers_for_station - actual_alight) 
              << " passengers could not alight" << std::endl;
  }

  this->passengers_by_destination[current_station] -= actual_alight;
  if (this->passengers_by_destination[current_station] <= 0) {
    this->passengers_by_destination.erase(current_station);
  }

  this->passenger_count -= actual_alight;
  return actual_alight;
}

int Train::board(int current_station, int count, const std::map<int, double>& destination_probs) {
  int capacity_available = get_remaining_capacity();
  int actual_board = std::min(count, capacity_available);

  if (actual_board <= 0) {
    return 0;
  }

  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(0.0, 1.0);

  std::vector<std::pair<int, double>> cumulative_probs;
  double cumsum = 0.0;
  for (const auto& pair : destination_probs) {
    cumsum += pair.second;
    cumulative_probs.push_back({pair.first, cumsum});
  }

  for (int i = 0; i < actual_board; ++i) {
    double rand_val = dis(gen);
    int destination = current_station;

    for (const auto& pair : cumulative_probs) {
      if (rand_val <= pair.second) {
        destination = pair.first;
        break;
      }
    }

    this->passengers_by_destination[destination]++;
  }

  this->passenger_count += actual_board;
  return actual_board;
}

Train::~Train() {
  this->capacity = 0;
  this->startIndex = 0;
  this->endIndex = 0;
  this->direction = 0;
  this->passengers_by_destination.clear();
}