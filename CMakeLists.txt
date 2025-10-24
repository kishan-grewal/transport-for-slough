cmake_minimum_required(VERSION 3.10)

project(tfs VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(${PROJECT_NAME} src/main.cpp src/event_queue.cpp src/event.cpp src/platform.cpp src/station.cpp src/train.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC include/)