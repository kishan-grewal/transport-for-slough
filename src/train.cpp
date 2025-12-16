#include "train.hpp"
#include "ode/json_util.hpp"

Train::Train(int passenger_count, int startIndex, int endIndex, int direction,
             boost::json::object& probability_mapping)
    : probability_mapping(probability_mapping) {
  this->passenger_count = passenger_count;
  this->startIndex = startIndex;
  this->endIndex = endIndex;
  this->direction = direction;

  this->passengers = std::map<std::string, int>();
  for (auto key : keys) {
    this->passengers.emplace(key, 0);
  }
}

int Train::try_embark(int n, const std::string& station) {
  if (n == 0)
    return 0;
  std::cout << "Embark" << std::endl;

  int successful = n;
  this->passenger_count += n;
  if (this->passenger_count > this->MAX_CAPACITY) {
    successful -= this->passenger_count - this->MAX_CAPACITY;
    this->passenger_count = this->MAX_CAPACITY;
  }

  // Distribute the passengers
  auto mapping = this->probability_mapping.at(this->direction == 1 ? "NB" : "SB")
                   .as_object()
                   .at("MON")
                   .at(station)
                   .as_object();
  std::vector<double> probs = std::vector<double>(sizeof(this->keys) / sizeof(this->keys[0]), 0);
  // Accumulate probabilities
  double acc = 0, tmp = 0;
  for (int i = 0; i < probs.size(); ++i) {
    const std::string& key = this->keys[i];
    if (mapping.contains(key)) {
      JSON_ParseNumericToDouble(tmp, &mapping.at(key));
      acc += tmp;
    }

    probs[i] = acc;
  }

  for (int i = 0; i < n; ++i) {
    double r = ((double)rand()) / RAND_MAX;
    for (int j = 0; j < probs.size(); ++j) {
      if (r > probs[j])
        continue;

      this->passengers[this->keys[i]] += 1;
      break;
    }
  }

  return successful;
}
int Train::disembark(const std::string& station) {
  int n = this->passengers[station];
  this->passengers[station] = 0;
  return n;
}

int Train::getStartIndex() const { return this->startIndex; }

int Train::getEndIndex() const { return this->endIndex; }

int Train::getDirection() const { return this->direction; }

void Train::setDirection(int newDirection) { this->direction = newDirection; }

Train::~Train() {
  this->passenger_count = 0;
  this->startIndex = 0;
  this->endIndex = 0;
  this->direction = 0;

  this->passengers.erase(this->passengers.begin(), this->passengers.end());
}