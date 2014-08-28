
cmake_minimum_required(VERSION 2.8)

if(WIN32)
    set(
        CMAKE_USER_MAKE_RULES_OVERRIDE
        "${CMAKE_CURRENT_SOURCE_DIR}/msvc_override.cmake"
    )
endif(WIN32)

project(vtrc)

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
              REQUIRED)
find_package( Protobuf REQUIRED)

set( VTRC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include )


if( NOT DEFINED VTRC_PROTOCOL_DIR )

    set( VTRC_PROTOCOL_INCLUDE_DIR ${VTRC_INCLUDE_DIR}/vtrc-common/protocol )

else( )

    set( VTRC_PROTOCOL_INCLUDE_DIR ${VTRC_PROTOCOL_DIR} )

endif( )

message( "Protocol files' path: " ${VTRC_PROTOCOL_INCLUDE_DIR} )

set( VTRC_INCLUDE_DIRS
     ${VTRC_INCLUDE_DIR}
     ${VTRC_INCLUDE_DIR}/vtrc-common/config
     ${VTRC_PROTOCOL_INCLUDE_DIR}
   )

include_directories( ${PROJECT_SOURCE_DIR} ${VTRC_INCLUDE_DIRS} )

add_subdirectory(vtrc-common)
add_subdirectory(vtrc-server)
add_subdirectory(vtrc-client-base)


if(MSVC)
    # Force to always compile with W4
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string( REGEX REPLACE "/W[0-4]" "/W4"
                CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else( )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif( )
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    # Update if necessary
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif( )

if(WITH_EXAMPLES)
    add_subdirectory(examples/calculator)
    add_subdirectory(examples/remote-fs)
    add_subdirectory(examples/lukki-db)
    add_subdirectory(examples/stress)
endif(WITH_EXAMPLES)



