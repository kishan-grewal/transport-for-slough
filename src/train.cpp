#include "train.hpp"

Train::Train(int capacity, int startIndex, int endIndex, int direction) {
    this->capacity = capacity;
    this->startIndex = startIndex;
    this->endIndex = endIndex;
    this->direction = direction;
}

int Train::getCapacity() const {
    return this->capacity;
}

int Train::getStartIndex() const {
    return this->startIndex;
}

int Train::getEndIndex() const {
    return this->endIndex;
}

int Train::getDirection() const {
    return this->direction;
}