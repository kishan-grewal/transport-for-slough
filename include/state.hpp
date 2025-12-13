#ifndef STATE_HPP
#define STATE_HPP

#include <vector>
#include <memory>
#include "station.hpp"
#include "train.hpp"
#include "data_loader.hpp"

class State {
public:
    std::vector<Station> stations;
    std::vector<Train> trains;
    std::shared_ptr<SimulationData> sim_data;
    
    const Train& getTrain(int index) const {
        return this->trains[index];
    }
    
    Train& getTrainMutable(int index) {
        return this->trains[index];
    }

    void changeTrainDirection(int index) {
        int cur = this->trains[index].getDirection();
        this->trains[index].setDirection(-cur);
    }
    
};

#endif