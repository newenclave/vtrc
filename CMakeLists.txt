
if(WIN32)
    set(
        CMAKE_USER_MAKE_RULES_OVERRIDE
        "${CMAKE_CURRENT_SOURCE_DIR}/msvc_override.cmake"
    )
endif(WIN32)

cmake_minimum_required(VERSION 2.8)

project(vtrc)

include( cxx11_check.cmake )

set( VTRC_CXX11_ENABLED 0 )

if( NOT VTRC_DISABLE_CXX11 )
    check_cxx11( VTRC_CXX11_ENABLED )
endif( )

set( VTRC_LINK_TYPE_STATIC STATIC )
set( VTRC_LINK_TYPE_SHARED SHARED )
set( VTRC_LINK_TYPE_MODULE MODULE )

if( NOT DEFINED VTRC_LINK_TYPE )
    set( VTRC_LINK_TYPE STATIC )
endif( )

###################### OPEN SSL ######################
#
# OPENSSL_ROOT_DIR - Set this variable to the root installation of OpenSSL
#
# Read-Only variables:
#
# OPENSSL_FOUND - system has the OpenSSL library
# OPENSSL_INCLUDE_DIR - the OpenSSL include directory
# OPENSSL_LIBRARIES - The libraries needed to use OpenSSL
# OPENSSL_VERSION - This is set to $major.$minor.$revision$path (eg. 0.9.8s)

find_package(OpenSSL)

set(VTRC_OPENSSL_FOUND 0)

if( OPENSSL_FOUND )
    set( VTRC_OPENSSL_FOUND 1 )
    #small hack =)
    set( OPENSSL_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR} )
    message("OpenSSL dir: " ${OPENSSL_INCLUDE_DIRS})

else(OPENSSL_FOUND)
    # clean up variables
    set(LIB_EAY_LIBRARY)
    set(SSL_EAY_LIBRARY)
    set(OPENSSL_LIBRARIES)
    set(OPENSSL_INCLUDE_DIRS)
endif( OPENSSL_FOUND )
#######################################################


if( ${VTRC_LINK_TYPE} MATCHES ${VTRC_LINK_TYPE_STATIC} )
    set(Boost_USE_STATIC_LIBS     ON)
    set(Boost_USE_STATIC_RUNTIME  ON)
endif( )

set( Boost_USE_MULTITHREADED   ON)

if( VTRC_CXX11_ENABLED )

    find_package( Boost 1.53 COMPONENTS
                        system
                        thread  # for the shared mutexes
                  REQUIRED)

    set( VTRC_DISABLE_CXX11 0 )

    get_cmd_cxx11( VTRC_CXX_CMD )
    get_compiler_thread_local( VTRC_THREAD_LOCAL )

    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VTRC_CXX_CMD}")

else( )

    find_package( Boost 1.53 COMPONENTS
                        system
                        chrono
                        date_time
                        regex
                        thread
                        atomic
                        random
                  REQUIRED)

    set( VTRC_DISABLE_CXX11 1 )

endif( )


find_package( Protobuf REQUIRED)

set( VTRC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include )
set( VTRC_CONFIG_INCLUDE_DIR ${VTRC_INCLUDE_DIR}/vtrc-common/config )

configure_file ("${PROJECT_SOURCE_DIR}/vtrc-general-config.h.in"
                "${VTRC_CONFIG_INCLUDE_DIR}/vtrc-general-config.h" )

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
     ${PROTOBUF_INCLUDE_DIRS}
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

message( "Compiler ID: "        ${CMAKE_CXX_COMPILER_ID} )
message( "Compiler version: "   ${CMAKE_CXX_COMPILER_VERSION} )
message( "CMAKE_CXX_FLAGS: "    ${CMAKE_CXX_FLAGS} )
message( "Link type: "          ${VTRC_LINK_TYPE} )

set( VTRC_COMMON_LIB_NAME vtrc_common )
set( VTRC_SERVER_LIB_NAME vtrc_server )
set( VTRC_CLIENT_LIB_NAME vtrc_client )

add_subdirectory(vtrc-common)
add_subdirectory(vtrc-server)
add_subdirectory(vtrc-client)

if(WITH_EXAMPLES)
    #add_subdirectory(examples/calculator)
    add_subdirectory( examples/remote-fs    )
    add_subdirectory( examples/lukki-db     )
    add_subdirectory( examples/stress       )
    add_subdirectory( examples/hello        )
    add_subdirectory( examples/hello-events )

if(OPENSSL_FOUND)
    add_subdirectory( examples/hello-ssl-handshake )
endif(OPENSSL_FOUND)
if(MSVC)
    add_subdirectory( examples/hello-win-impersonate )
endif(MSVC)

    add_subdirectory( examples/proxy-test   )

endif(WITH_EXAMPLES)

configure_file ("${PROJECT_SOURCE_DIR}/vtrc-import.cmake.in"
                "${PROJECT_SOURCE_DIR}/vtrc-import.cmake" )

if( CUSTOM_CONFIG )

    if( CUSTOM_CONFIG_OUTPUT )
        set( custom_config_output ${CUSTOM_CONFIG_OUTPUT} )
    else( )
        # /full/path/to/file.ext.in -> /full/path/to/file.ext
        string( REGEX REPLACE "\\.[^\\..]+$" ""
                custom_config_output ${CUSTOM_CONFIG} )
    endif( )

    message( "Configuring custom config..."
             "${CUSTOM_CONFIG} -> ${custom_config_output}")

    configure_file ( "${CUSTOM_CONFIG}" "${custom_config_output}" )

endif( )


