#include <iostream>

#include "event_loop.hpp"
#include "platform.hpp"
#include "station.hpp"
#include "train.hpp"

int main(int argc, char** argv) {
    std::cout << "Hello World!" << std::endl;

    int simTime = 100000; //in seconds
    State initialState;
    initialState.stations = std::vector{Station("Stratford"),Station("West Ham"),Station("Canning Town"),Station("North Greenwich"),Station("Canary Wharf"), Station("Canada Water"),
                      Station("Bermondsey"),Station("London Bridge"),Station("Southwark"),Station("Waterloo"),Station("Westminster"),Station("Green Park"),
                      Station("Bond Street"), Station("Baker Street"), Station("St John's Wood"), Station("Swiss Cottage"), Station("Finchley Road"), Station("West Hampstead"),
                      Station("Kilburn"), Station("Willesden Green"), Station("Dollis Hill"), Station("Neasden"), Station("Wembley Park"), Station("Kingsbury"), Station("Queensbury"),
                      Station("Canons Park"), Station("Stanmore")};

    initialState.trains = std::vector{Train(100, 0, 0, 1), Train(100, 1, 10, 1), Train(100, 2, 20, 1), Train(100, 3, 20, 1), Train(100, 4, 20, 1), Train(100, 5, 10, 1), Train(100, 6, 20, 1), Train(100, 7, 20, 1), Train(100, 8, 20, 1)};
    EventLoop loop = EventLoop(simTime, initialState);
    loop.start();
    std::cin.get();
    loop.waitBarrier();

    std::cout << "Program finished successfully." << std::endl;

    return 0;
}