
if(WIN32)
    set(
        CMAKE_USER_MAKE_RULES_OVERRIDE
        "${CMAKE_CURRENT_SOURCE_DIR}/msvc_override.cmake"
    )
endif(WIN32)

cmake_minimum_required(VERSION 2.8)

set(INTERNAL_PROJECT_NAME proxy_test )

project(vtrc-${INTERNAL_PROJECT_NAME})

if( ${VTRC_LINK_TYPE} MATCHES ${VTRC_LINK_TYPE_STATIC} )
    set(Boost_USE_STATIC_LIBS     ON)
    set(Boost_USE_STATIC_RUNTIME  ON)
endif( )

set(Boost_USE_MULTITHREADED   ON)

if( VTRC_CXX11_ENABLED )

    find_package( Boost 1.50 COMPONENTS
                    system
                    thread
                  REQUIRED)

else( )
    find_package( Boost 1.50 COMPONENTS
                    system
                    chrono
                    thread
                    atomic
                    random
                  REQUIRED)
endif( )

find_package( Protobuf REQUIRED)

include_directories( ${VTRC_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} )

add_subdirectory(protocol)
add_subdirectory(client)
add_subdirectory(server)

