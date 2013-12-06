
cmake_minimum_required(VERSION 2.8)

project(vtrc)

set(Boost_USE_STATIC_LIBS     ON)
set(Boost_USE_MULTITHREADED   ON)
set(Boost_USE_STATIC_RUNTIME  ON)

find_package( Boost 1.50 COMPONENTS system date_time regex REQUIRED)
find_package( Protobuf REQUIRED)

add_subdirectory(vtrc-common)
add_subdirectory(vtrc-server)
add_subdirectory(vtrc-test-server)
