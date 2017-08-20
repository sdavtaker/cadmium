cmake_minimum_required(VERSION 3.6.1)
project(Cadmium)
# The version number.
set (Cadmium_VERSION_MAJOR 0)
set (Cadmium_VERSION_MINOR 0)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_library(Cadmium INTERFACE)

include_directories(include)

#target_include_directories(Cadmium INTERFACE
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#  $<INSTALL_INTERFACE:include/Cadmium>
#)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -std=c++14")

FILE(GLOB TestSources test/*.cpp)
add_executable(test_executable ${TestSources})
add_executable(clock_example example/main-clock.cpp)
target_include_directories(test_executable PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(test_executable ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

enable_testing()
add_test(unit_test test_executable)