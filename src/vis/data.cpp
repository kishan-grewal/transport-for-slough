#include "vis/data.hpp"
#include <fstream>
#include <iostream>

StationCSV::StationCSV(std::string filename) {
    std::ifstream file(filename);

    std::string firstLine = "";
    std::getline(file, firstLine);
    std::vector<std::string> fields;
    std::istringstream fline(firstLine);

    std::string field;

    while(std::getline(fline,field,',')) {
        fields.push_back(field);
    }

    this->platforms = fields.size() - 1;

    std::vector<int> platStatus = {};

    while(std::getline(file, firstLine)) {
        fline = std::istringstream(firstLine);
        std::getline(fline, field, ',');
        this->population.push_back(std::stoi(field));
        platStatus = {};
        for(int i = 0; i < this->platforms; ++i) {
            std::getline(fline,field,',');
            platStatus.push_back(std::stoi(field));
        }
        this->busy.push_back(platStatus);
    }

    file.close();
}

int StationCSV::getPop(int idx) {
    return this->population[idx];
}

void StationCSV::printData(int idx) {
    std::string plats = "";
    for(int i = 0; i < this->platforms; ++i) {
        plats += std::to_string(this->busy[idx][i]) + " ";
    }
    std::cout << population[idx] << " " << plats << std::endl;
}

StationCSV::~StationCSV() = default;

std::vector<std::string> getData(std::string root) {
    std::vector<std::string> fnames = {};
    for(const auto& entry : std::filesystem::directory_iterator(root)) {
        fnames.push_back(entry.path());
    }

    return fnames;
}

std::vector<int> getTimes(std::string fname) {
    std::string first = "";
    std::fstream file(fname);

    std::getline(file, first);
    std::vector<int> ts = {};

    while(std::getline(file, first)) {
        ts.push_back(std::stoi(first));
    }

    return ts;
}
