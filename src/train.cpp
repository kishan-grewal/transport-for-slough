#include "train.hpp"

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

Train::~Train() {
  this->capacity = 0;
  this->startIndex = 0;
  this->endIndex = 0;
  this->direction = 0;
}