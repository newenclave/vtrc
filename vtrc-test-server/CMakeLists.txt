
aux_source_directory(. src_dir)

set( exe_name vtrc_test_server)

add_executable(${exe_name} ${src_dir})

list(APPEND depends  
	vtrc_common_s
	vtrc_server_s
	)

foreach( dep depends )

	add_dependencies(       ${exe_name} ${dep}      )
	add_definitions(        ${${dep}_DEFINITIONS}   )
	include_directories(    ${${dep}_INCLUDE_DIRS}  )
	target_link_libraries(  ${${dep}_LIBRARIES}     )
	
endforeach()


set_target_properties(${exe_name} PROPERTIES COMPILE_DEFINITIONS "STATIC")
target_link_libraries(${exe_name} ${PROTOBUF_LIBRARIES})

if(WIN32)
	target_link_libraries(${exe_name} ws2_32.lib)
else(WIN32)
    target_link_libraries(${Boost_LIBRARIES})
endif(WIN32)

