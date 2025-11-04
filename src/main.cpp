#include <iostream>

#include "event_loop.hpp"
#include "platform.hpp"
#include "station.hpp"
#include "train.hpp"

int main(int argc, char** argv) {
    std::cout << "Hello World!" << std::endl;

    int simTime = 100000; //in seconds
    EventLoop loop = EventLoop(simTime);
    loop.start();
    std::cin.get();

    std::cout << "Program finished successfully." << std::endl;

    return 0;
}