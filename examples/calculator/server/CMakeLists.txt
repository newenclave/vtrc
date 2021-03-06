
aux_source_directory(. src_dir)

set( exe_name ${INTERNAL_PROJECT_NAME}_server )

add_executable(${exe_name} ${src_dir})

list( APPEND internal
    ${INTERNAL_PROJECT_NAME}_protocol_s
    vtrc_server_s
    vtrc_common_s
    )

include_directories( ${${INTERNAL_PROJECT_NAME}_protocol_s_INCLUDE_DIRS}  )

message( "-- Seting up internal dependencies for " ${exe_name} )
foreach( dep ${internal} )

    message( "\tAdd dependence: " ${dep} )

    add_dependencies(       ${exe_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    target_link_libraries(  ${exe_name} ${${dep}_LIBRARIES} )

endforeach(dep)

list(APPEND dependencies
    Boost
    PROTOBUF
    )

target_link_libraries(  ${exe_name} ${vtrc_server_s_LIBRARIES} )
target_link_libraries(  ${exe_name} ${vtrc_common_s_LIBRARIES} )

message( "-- Seting up dependencies for " ${exe_name} )

foreach( dep ${dependencies} )

    message( "\tAdd dependence: " ${dep} )

    add_dependencies(       ${exe_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    include_directories(    ${${dep}_INCLUDE_DIRS}  )
    target_link_libraries(  ${exe_name} ${${dep}_LIBRARIES} )

endforeach(dep)

#message( ${vtrc_server_s_LIBRARIES} ", " ${vtrc_common_s_LIBRARIES} )

if(WIN32)
    target_link_libraries(${exe_name} ws2_32.lib)
endif(WIN32)

set_target_properties(${exe_name} PROPERTIES COMPILE_DEFINITIONS "STATIC")

