
aux_source_directory(. src_dir)

set( exe_name remote_fs_server )

add_executable(${exe_name} ${src_dir})

list(APPEND dependencies
    remote_fs_protocol_s
    vtrc_server_s
    vtrc_common_s
    Boost
    PROTOBUF
    )

include_directories(${vtrc_common_s_PROJECT_SOURCE_DIR})

message( "-- Seting up dependencies for " ${exe_name} )
foreach( dep ${dependencies} )

    message( "\tAdd dependence: " ${dep} )

    add_dependencies(       ${exe_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    include_directories(    ${${dep}_INCLUDE_DIRS}  )
    target_link_libraries(  ${exe_name} ${${dep}_LIBRARIES} )

endforeach(dep)

if(WIN32)
    target_link_libraries(${exe_name} ws2_32.lib)
endif(WIN32)

set_target_properties(${exe_name} PROPERTIES COMPILE_DEFINITIONS "STATIC")

