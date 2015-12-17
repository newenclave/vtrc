set( lib_name ${VTRC_SERVER_LIB_NAME} )

list(APPEND src_dirs 
        .
        ${VTRC_INCLUDE_DIR}/vtrc-server
    )

foreach( src_dir ${src_dirs} )
    aux_source_directory(${src_dir} lib_src)
    file(GLOB headers ${src_dir}/*.h)
    list(APPEND lib_src ${headers})
endforeach( )

include_directories( ${VTRC_INCLUDE_DIR}/vtrc-server)

add_library(${lib_name} ${VTRC_LINK_TYPE} ${lib_src})

if( OPENSSL_FOUND )
    list( APPEND dependencies  OPENSSL )
endif( OPENSSL_FOUND )

list(APPEND dependencies  
    ${VTRC_COMMON_LIB_NAME}
    Boost
    PROTOBUF
    )

message( "-- Seting up dependencies for " ${lib_name} )
foreach( dep ${dependencies} )
    
    message( "\tAdd dependence: " ${dep} )
    
    add_dependencies(       ${lib_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    include_directories(    ${${dep}_INCLUDE_DIRS}  )
    #target_link_libraries(  ${lib_name} ${${dep}_LIBRARIES} )
    
endforeach(dep)

set_target_properties(${lib_name}
                      PROPERTIES
                        COMPILE_DEFINITIONS "${VTRC_LINK_TYPE}")
get_target_property(${lib_name}_LOCATION ${lib_name} LOCATION)

set(${lib_name}_DEFINITIONS "-D${VTRC_LINK_TYPE}" PARENT_SCOPE)
set(${lib_name}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

set(${lib_name}_LIBRARIES ${${lib_name}_LOCATION}
                          ${${VTRC_COMMON_LIB_NAME}_LIBRARIES} PARENT_SCOPE )
