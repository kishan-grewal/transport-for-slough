cmake_minimum_required(VERSION 3.10)

project(tfs VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=thread")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")

find_package(SFML 3.0.2 COMPONENTS System Window Graphics Network Audio QUIET)

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

#add_subdirectory(libs/SFGraphing)

add_executable(${PROJECT_NAME} 
  src/main.cpp
  src/event_pool.cpp
  src/event.cpp
  src/platform.cpp
  src/station.cpp
  src/train.cpp
  src/event_loop.cpp

  #src/odeint_tests.cpp
  #src/ode/ode_solver.cpp
)

add_executable(${PROJECT_NAME}_visualiser
  src/vis/main.cpp
  src/vis/data.cpp)

target_include_directories(${PROJECT_NAME}_visualiser PUBLIC include/)
target_include_directories(${PROJECT_NAME} PUBLIC include/)
target_include_directories(${PROJECT_NAME} PUBLIC libs/boost/include) # libs/SFGraphing/include)

# Link SFGraphing fork (which builds with SFML exposed publically)
#target_link_libraries(${PROJECT_NAME} SFGraphing)
if(SFML_FOUND)
    message(STATUS "Found SFML version: ${SFML_VERSION}")
    
    # SFML 3 uses namespaced targets (e.g., SFML::System)
    # This checks if the target exists, which is the modern way to link
    if(TARGET SFML::System)
        message(STATUS "Linking against SFML targets")
        target_link_libraries(tfs_visualiser PRIVATE SFML::System SFML::Window SFML::Graphics SFML::Network SFML::Audio)
    else()
        # Fallback for older SFML versions that use variables
        message(STATUS "Linking against SFML variables")
        target_link_libraries(tfs_visualiser PRIVATE ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
        target_include_directories(tfs_visualiser PRIVATE ${SFML_INCLUDE_DIRS})
    endif()
endif()