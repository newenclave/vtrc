
set(lib_name vtrc_common_lib_s)

aux_source_directory(. common_src)

file(GLOB headers *.h)
list(APPEND common_src ${headers})

include_directories(
    ${PROJECT_SOURCE_DIR} 
    )

add_library(${lib_name} STATIC ${common_src})
#add_dependencies(${lib_name} vtrc_proto_lib_s)

set_target_properties(${lib_name} PROPERTIES COMPILE_DEFINITIONS "STATIC")
get_target_property(${lib_name}_location ${lib_name} LOCATION)

set(${lib_name}_DEFINITIONS "-DSTATIC" PARENT_SCOPE)
set(${lib_name}_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(${lib_name}_LIBRARIES ${${lib_name}_location} PARENT_SCOPE)
