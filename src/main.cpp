#include <iostream>
#include <vector>
#include <string>

#include "event_loop.hpp"
#include "platform.hpp"
#include "station.hpp"
#include "train.hpp"

<<<<<<< HEAD
int main(int argc, char** argv) {
    std::cout << "Hello World!" << std::endl;

    int simTime = 100000; //in seconds
    State initialState;
    std::vector<std::string> stationNames = {"Stratford","West Ham","Canning Town","North Greenwich","Canary Wharf","Canada Water",
                      "Bermondsey","London Bridge","Southwark","Waterloo","Westminster","Green Park",
                      "Bond Street","Baker Street","St John's Wood","Swiss Cottage","Finchley Road","West Hampstead",
                      "Kilburn","Willesden Green","Dollis Hill","Neasden","Wembley Park","Kingsbury","Queensbury",
                      "Canons Park","Stanmore","Turnaround point"};

    
    initialState.stations = std::vector<Station>();
    initialState.stations.reserve(stationNames.size());
    for(int i = 0; i < stationNames.size(); ++i) {
        initialState.stations.emplace_back(stationNames[i]);
    }

    initialState.trains = std::vector<Train>();
    initialState.trains.reserve(10);
    for(int i = 0; i < 10; ++i) {
        initialState.trains.emplace_back(100, i, 10, 1);
    }
    EventLoop loop = EventLoop(simTime, std::move(initialState), initialState.stations.size());
    loop.start();
    std::cin.get();

    std::cout << "Program finished successfully." << std::endl;

    return 0;
=======
#include <boost/numeric/odeint.hpp>

int main(int argc, char **argv) {
  std::cout << "Hello World!" << std::endl;
  return 0;
>>>>>>> 58cc8be5ed5a2f5a772f7d79d0ca06d7214177ef
}