#include "train.hpp"

//! Train constructor
/*!
Set the intial conditions for a train
\param capacity int maximum train capacity
\param startIndex int first station on route
\param endIndex int last station on route
\param direction int current train travelling direction (1 for forward out of Stratford, -1 backward toward Stratford)
\param passenger_count int number of passengers currently on the train
*/
Train::Train(int capacity, int startIndex, int endIndex, int direction, int passenger_count) {
  this->capacity = capacity;
  this->startIndex = startIndex;
  this->endIndex = endIndex;
  this->direction = direction;
  this->passenger_count = passenger_count;
}

//! Train capcity getter
int Train::getCapacity() const { return this->capacity; }

//! Train route start getter
int Train::getStartIndex() const { return this->startIndex; }

//! Train route end getter
int Train::getEndIndex() const { return this->endIndex; }

//! Train direction getter
int Train::getDirection() const { return this->direction; }

//! Train direction setter
void Train::setDirection(int newDirection) { this->direction = newDirection; }

//! Train destructor
Train::~Train() {
  this->capacity = 0;
  this->startIndex = 0;
  this->endIndex = 0;
  this->direction = 0;
}