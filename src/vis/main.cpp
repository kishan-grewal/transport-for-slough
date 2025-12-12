#include <iostream>
#include <thread>
#include <SFML/Graphics.hpp>
#include "vis/data.hpp"

int main(int argc, char** argv) {
    std::cout << "hi" << std::endl;
    std::vector<std::string> files = getData("./out");
    std::vector<int> times = {};
    std::vector<StationCSV> stations = {};
    std::vector<sf::Text> texts = {};
    int j = 0;
    sf::Font font("arial.ttf");

    std::vector<std::string> order {"./out/Stratford.csv", "./out/West Ham.csv", "./out/Canning Town.csv", "./out/North Greenwich.csv", "./out/Canary Wharf.csv",
                                    "./out/Canada Water.csv", "./out/Bermondsey.csv", "./out/London Bridge.csv", "./out/Southwark.csv", "./out/Waterloo.csv",
                                    "./out/Westminster.csv", "./out/Green Park.csv", "./out/Bond Street.csv", "./out/Baker Street.csv", "./out/St John's Wood.csv",
                                    "./out/Swiss Cottage.csv", "./out/Finchley Road.csv", "./out/West Hampstead.csv", "./out/Kilburn.csv", "./out/Willesden Green.csv",
                                    "./out/Dollis Hill.csv", "./out/Neasden.csv", "./out/Wembley Park.csv", "./out/Kingsbury.csv", "./out/Queensbury.csv", "./out/Canons Park.csv",
                                    "./out/Stanmore.csv", "./out/Turnaround point.csv", "./out/time.csv"};


    files = order;

    for(int i = 0; i < files.size(); ++i) {
        std::cout << files[i] << std::endl;
        if(files[i] != "./out/time.csv") {
            stations.push_back(StationCSV(files[i]));
            stations[j].printData(2);
            
            texts.push_back(sf::Text(font));
            texts[j].setString(files[i].substr(6));

            ++j;

        }
        else {
            times = getTimes(files[i]);
        }
    }

    sf::RenderWindow window(sf::VideoMode({2000,600}), "TFS");

    std::vector<sf::CircleShape> stationMarkers = {};
    std::vector<sf::CircleShape> popMarkers = {};
    sf::RectangleShape line({1800, 20});
    line.setFillColor(sf::Color(0xA0A5A9FF));
    line.setPosition({40,20});

    sf::Text timer(font);
    timer.setCharacterSize(20);
    timer.setFillColor(sf::Color::Black);
    timer.setPosition({20, 400});
    for(int i = 0; i < stations.size(); ++i) {
        stationMarkers.push_back(sf::CircleShape(10.f));
        stationMarkers[i].setFillColor(sf::Color::White);
        stationMarkers[i].setOutlineColor(sf::Color::Black);
        stationMarkers[i].setOutlineThickness(2.f);
        stationMarkers[i].setPosition({30+100*i, 20});

        popMarkers.push_back(sf::CircleShape(10.f));
        popMarkers[i].setFillColor(sf::Color::Red);
        popMarkers[i].setPosition({30+100*i, 80});

        texts[i].setCharacterSize(10);
        texts[i].setFillColor(sf::Color::Black);
        texts[i].setPosition({30+100*i, 50});
    }

    int cnt = 0;
    while(window.isOpen() && cnt <= times.size()) {
        timer.setString(std::to_string(times[cnt]));
        while(const std::optional event = window.pollEvent()) {
            if(event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        window.clear(sf::Color::White);
        window.draw(line);
        window.draw(timer);

        for(int i = 0; i < stationMarkers.size(); ++i) {
            popMarkers[i].setRadius(stations[i].getPop(cnt)/5000);
            //popMarkers[i].setFillColor(sf::Color(0x990000));
            window.draw(stationMarkers[i]);
            window.draw(texts[i]);
            window.draw(popMarkers[i]);
        }
        
        window.display();
        cnt++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}