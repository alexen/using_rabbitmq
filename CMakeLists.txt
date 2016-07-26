cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
include_directories(${CMAKE_SOURCE_DIR} SYSTEM /opt/itcs/include/)

find_package(Rabbitmq REQUIRED)
set(BOOST_ROOT /opt/itcs)
find_package(Boost REQUIRED thread system)

add_subdirectory(rabbitmq_client)
add_subdirectory(producer)
add_subdirectory(consumer)
add_subdirectory(producer_consumer)