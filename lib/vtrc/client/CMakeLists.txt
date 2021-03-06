
set( lib_name ${VTRC_CLIENT_LIB_NAME} )

list(APPEND src_dirs
        .
        ./unix
        ./win
        ${VTRC_INCLUDE_DIR}/vtrc/client
    )

foreach( src_dir ${src_dirs} )
    aux_source_directory(${src_dir} lib_src)
    file(GLOB headers ${src_dir}/*.h)
    list(APPEND lib_src ${headers})
endforeach( )

add_library(${lib_name} ${VTRC_LINK_TYPE} ${lib_src})

list( APPEND dependencies
      ${VTRC_COMMON_LIB_NAME}
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

set(${lib_name}_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${${VTRC_COMMON_LIB_NAME}_INCLUDE_DIRS}
    PARENT_SCOPE
)

set(${lib_name}_LIBRARY ${${lib_name}_LOCATION} PARENT_SCOPE )

set(${lib_name}_LIBRARIES
    ${${lib_name}_LOCATION}
    ${${VTRC_COMMON_LIB_NAME}_LIBRARIES}
    PARENT_SCOPE
)

