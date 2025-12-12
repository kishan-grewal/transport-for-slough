cmake_minimum_required(VERSION 3.10)

project(tfs VERSION 0.1.0)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall")

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

add_subdirectory(libs/SFGraphing)

add_executable(${PROJECT_NAME}
  src/odeint_tests.cpp
  src/ode/ode_solver.cpp
  src/station_ode.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC include/)
target_include_directories(${PROJECT_NAME} PUBLIC libs/boost/include libs/SFGraphing/include)

# Link SFGraphing fork (which builds with SFML exposed publically)
target_link_libraries(${PROJECT_NAME} SFGraphing)