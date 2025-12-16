#ifndef TRAIN_HPP
#define TRAIN_HPP

#include <map>
#include <string>
#include <boost/json.hpp>

class Train {
  private:
  int passenger_count;
  std::map<std::string, int> passengers;
  boost::json::object& probability_mapping;
  int startIndex;
  int endIndex;
  int direction;

  constexpr static int MAX_CAPACITY = 1000;
  constexpr static std::string keys[] = {
    "Stratford",     "West Ham",      "Canning Town",   "North Greenwich", "Canary Wharf",
    "Canada Water",  "Bermondsey",    "London Bridge",  "Southwark",       "Waterloo",
    "Westminster",   "Green Park",    "Bond Street",    "Baker Street",    "St. John's Wood",
    "Swiss Cottage", "Finchley Road", "West Hampstead", "Kilburn",         "Willesden Green",
    "Dollis Hill",   "Neasden",       "Wembley Park",   "Kingsbury",       "Queensbury",
    "Canons Park",   "Stanmore"};

  public:
  Train(int passenger_count, int startIndex, int endIndex, int direction,
        boost::json::object& probability_mapping);
  Train(const Train& cpy)
      : passenger_count(cpy.passenger_count),
        startIndex(cpy.startIndex),
        endIndex(cpy.endIndex),
        direction(cpy.direction),
        passengers(cpy.passengers),
        probability_mapping(cpy.probability_mapping) {}
  int getId() const;
  int getStartIndex() const;
  int getEndIndex() const;
  int getDirection() const;
  void setDirection(int newDirection);

  int try_embark(int n, const std::string& station);
  int disembark(const std::string& station);
  ~Train();
};

#endif