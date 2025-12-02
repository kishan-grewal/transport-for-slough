#include <iostream>
#include <vector>
#include <string>

#include "event_loop.hpp"
#include "platform.hpp"
#include "station.hpp"
#include "train.hpp"

int main(int argc, char** argv) {
    std::cout << "Hello World!" << std::endl;
    int simTime = 10000;

    for(int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if(arg.substr(0,2) == "-t") {
            simTime = std::stoi(arg.substr(2));
        }
    }

    State initialState;
    std::vector<std::pair<std::string,std::vector<int>>> stationNames = {{"Stratford", {0, 0, 0}},{"West Ham", {1, -1}},{"Canning Town", {1, -1}},{"North Greenwich", {1, -1, -1}},{"Canary Wharf", {1, -1}},{"Canada Water", {1, -1}},
                      {"Bermondsey", {1, -1}},{"London Bridge", {1, -1}},{"Southwark", {1, -1}},{"Waterloo", {1,-1}},{"Westminster", {1,-1}},{"Green Park", {1,-1}},
                      {"Bond Street", {1,-1}},{"Baker Street", {1,-1}},{"St John's Wood", {1,-1}},{"Swiss Cottage", {1,-1}},{"Finchley Road",{1,-1}},{"West Hampstead",{1,-1}},
                      {"Kilburn",{1,-1}},{"Willesden Green",{1,-1}},{"Dollis Hill",{1,-1}},{"Neasden",{1,-1}},{"Wembley Park",{1,-1}},{"Kingsbury",{1,-1}},{"Queensbury",{1,-1}},
                      {"Canons Park",{1,-1}},{"Stanmore",{1,-1}},{"Turnaround point",{0,0,0}}};

    
    initialState.stations = std::vector<Station>();
    initialState.stations.reserve(stationNames.size());
    for(int i = 0; i < stationNames.size(); ++i) {
        initialState.stations.emplace_back(std::get<std::string>(stationNames[i]), std::get<std::vector<int>>(stationNames[i]));
    }

    initialState.trains = std::vector<Train>();
    initialState.trains.reserve(26);
    for(int i = 0; i < 25; ++i) {
        initialState.trains.emplace_back(100, i, 10, 1);
    }
    //initialState.trains.emplace_back(100, 0, 10, 1);
    EventLoop loop = EventLoop(simTime, std::move(initialState), initialState.stations.size());
    auto simStart = std::chrono::system_clock::now();
    loop.start();
    std::cin.get();
    auto simEnd = std::chrono::system_clock::now();
    auto simTimeReal = std::chrono::duration_cast<std::chrono::seconds>(simEnd - simStart);

    std::cout << "Simulation took: " << simTimeReal.count() << "seconds" << std::endl;

    std::cout << "Program finished successfully." << std::endl;

    return 0;
}