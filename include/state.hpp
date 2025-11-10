#ifndef STATE_HPP
#define STATE_HPP

#include <vector>
#include "station.hpp"
#include "train.hpp"

class State {
public:
    std::vector<Station> stations; //maybe make these into a separate "state" or "state manager" class to avoid unwanted access
    std::vector<Train> trains;
    const Train& getTrain(int index) const {
        return this->trains[index];
    }

    void changeTrainDirection(int index) {
        int cur = this->trains[index].getDirection();
        this->trains[index].setDirection(-cur);
    }
    
};

#endif