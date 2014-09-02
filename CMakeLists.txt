
cmake_minimum_required(VERSION 2.8)

if(WIN32)
    set(
        CMAKE_USER_MAKE_RULES_OVERRIDE
        "${CMAKE_CURRENT_SOURCE_DIR}/msvc_override.cmake"
    )
endif(WIN32)

project(vtrc)

set( VTRC_LINK_TYPE_STATIC STATIC )
set( VTRC_LINK_TYPE_SHARED SHARED )
set( VTRC_LINK_TYPE_MODULE MODULE )

if( NOT DEFINED VTRC_LINK_TYPE )
    set( VTRC_LINK_TYPE STATIC )
endif( )

if( ${VTRC_LINK_TYPE} MATCHES ${VTRC_LINK_TYPE_STATIC} )
    set(Boost_USE_STATIC_LIBS     ON)
    set(Boost_USE_STATIC_RUNTIME  ON)
endif( )

set(Boost_USE_MULTITHREADED   ON)

find_package( Boost 1.53 COMPONENTS
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
set( VTRC_CONFIG_INCLUDE_DIR ${VTRC_INCLUDE_DIR}/vtrc-common/config )

#
# VTRC_PROTOCOL_DIR
#   use this option to copy generated protocol files into desired directory
#
#   default is "${PROJECT_SOURCE_DIR}/include/vtrc-common/protocol"
#
#
if( NOT DEFINED VTRC_PROTOCOL_DIR )
    set( VTRC_PROTOCOL_INCLUDE_DIR ${VTRC_INCLUDE_DIR}/vtrc-common/protocol )
else( )
    set( VTRC_PROTOCOL_INCLUDE_DIR ${VTRC_PROTOCOL_DIR} )
endif( )


message( "Protocol files' path: " ${VTRC_PROTOCOL_INCLUDE_DIR} )

set( VTRC_INCLUDE_DIRS
     ${VTRC_INCLUDE_DIR}
     ${VTRC_CONFIG_INCLUDE_DIR}
     ${VTRC_PROTOCOL_INCLUDE_DIR}
   )

include_directories( ${PROJECT_SOURCE_DIR} ${VTRC_INCLUDE_DIRS} )

if(MSVC)
    # Force to always compile with W4
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string( REGEX REPLACE "/W[0-4]" "/W4"
                CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else( )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif( )
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
    # Update if necessary

    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")

elseif(  CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")

endif( )

add_subdirectory(vtrc-common)
add_subdirectory(vtrc-server)
add_subdirectory(vtrc-client)

if(WITH_EXAMPLES)

    add_subdirectory(examples/calculator)
    add_subdirectory(examples/remote-fs)
    add_subdirectory(examples/lukki-db)
    add_subdirectory(examples/stress)
    add_subdirectory(examples/hello)

endif(WITH_EXAMPLES)



