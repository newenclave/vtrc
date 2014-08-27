
if(WIN32)
    set(
        CMAKE_USER_MAKE_RULES_OVERRIDE
        "${CMAKE_CURRENT_SOURCE_DIR}/msvc_override.cmake"
    )
endif(WIN32)

cmake_minimum_required(VERSION 2.8)

project(vtrc-remote-fs)

set(Boost_USE_STATIC_LIBS     ON)
set(Boost_USE_MULTITHREADED   ON)
set(Boost_USE_STATIC_RUNTIME  ON)

find_package( Boost 1.50 COMPONENTS
                    system
                    chrono
                    date_time
                    regex
                    thread
                    atomic
                    random
                    program_options
                    filesystem
              REQUIRED)
find_package( Protobuf REQUIRED)

include_directories(${PROJECT_SOURCE_DIR} ${VTRC_INCLUDE_DIRS})

add_subdirectory(protocol)
add_subdirectory(client)
add_subdirectory(server)

