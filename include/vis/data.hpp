#ifndef DATA_HPP
#define DATA_HPP

#include <filesystem>
#include <vector>

class StationCSV {
private:
    std::vector<int> population;
    int platforms;
    std::vector<std::vector<int>> busy;

public:
    StationCSV(std::string filename);
    void readData();
    int getPop(int idx);
    void printData(int idx);
    ~StationCSV();
};


std::vector<std::string> getData(std::string root);

std::vector<int> getTimes(std::string fname);
#endif