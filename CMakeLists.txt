cmake_minimum_required(VERSION 3.10)

project(tfs VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ")

add_subdirectory(libs/SFGraphing)
add_executable(${PROJECT_NAME} src/main.cpp src/event_pool.cpp src/event.cpp src/platform.cpp src/station.cpp src/train.cpp src/event_loop.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC include/)
target_include_directories(${PROJECT_NAME} PUBLIC libs/SFGraphing/include/)

target_link_libraries(${PROJECT_NAME} PUBLIC SFGraphing)