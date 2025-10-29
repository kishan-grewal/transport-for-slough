cmake_minimum_required(VERSION 3.10)

project(tfs VERSION 0.1.0)

add_definitions(-DBOOST_NO_USER_CONFIG=1)

# Create compile_commands.json symlink for IDE to use
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (PROJECT_IS_TOP_LEVEL AND UNIX)
    # Create symlink to compile_commands.json in root directory for IDE to pick it up
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
    )
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(${PROJECT_NAME} 
  # src/main.cpp
  # src/event_queue.cpp
  # src/event.cpp
  # src/platform.cpp
  # src/station.cpp
  # src/train.cpp
  src/odeint_tests.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC include/)
target_include_directories(${PROJECT_NAME} PUBLIC libs/boost/include)