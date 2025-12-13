#ifndef TRAIN_HPP
#define TRAIN_HPP

#include <map>
#include <string>

class Train {
  private:
  int capacity;
  int startIndex;
  int endIndex;
  int direction;
  int passenger_count;

  std::map<int, int> passengers_by_destination;

  public:
  Train(int capacity, int startIndex, int endIndex, int direction, int passenger_count = 0);
  
  int getCapacity() const;
  int getStartIndex() const;
  int getEndIndex() const;
  int getDirection() const;
  void setDirection(int newDirection);
  
  int get_passenger_count() const;
  int get_remaining_capacity() const;
  int get_passengers_for_station(int station_index) const;
  
  int alight(int current_station, int max_platform_capacity);
  int board(int current_station, int count, const std::map<int, double>& destination_probs);
  
  ~Train();
};

#endif