cmake_minimum_required(VERSION 2.8)
project(cmetronome)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra -Wpedantic")
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(PulseAudio REQUIRED)
include_directories(${PULSEAUDIO_INCLUDE_DIRS})
add_executable(cmetronome main.cpp)
target_link_libraries(cmetronome ${PULSEAUDIOSIMPLE_LIBRARIES} ${PULSEAUDIO_LIBRARIES})
install(TARGETS cmetronome DESTINATION bin)
