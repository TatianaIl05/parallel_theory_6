cmake_minimum_required(VERSION 3.10)
project(HeatSolver)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -acc -Minfo=all -O3")

add_executable(heat1 step1.cpp)
add_executable(heat2 step2.cpp)
add_executable(heat3 step3.cpp)
add_executable(heat4 main.cpp)

target_link_libraries(heat1 ${Boost_LIBRARIES})
target_link_libraries(heat2 ${Boost_LIBRARIES})
target_link_libraries(heat3 ${Boost_LIBRARIES})
target_link_libraries(heat4 ${Boost_LIBRARIES})
