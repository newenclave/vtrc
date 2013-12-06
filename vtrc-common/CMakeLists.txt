
set(lib_name vtrc_common_s)

list(APPEND src_dirs 
        .
        ./hash
    )

foreach( src_dir ${src_dirs} )
    aux_source_directory(${src_dir} lib_src)
    file(GLOB headers ${src_dir}/*.h)
    list(APPEND lib_src ${headers})
endforeach( )

include_directories(
    ${PROJECT_SOURCE_DIR} 
    )

add_library(${lib_name} STATIC ${lib_src})
#add_dependencies(${lib_name} vtrc_proto_s)

set_target_properties(${lib_name} PROPERTIES COMPILE_DEFINITIONS "STATIC")
get_target_property(${lib_name}_LOCATION ${lib_name} LOCATION)

set(${lib_name}_DEFINITIONS "-DSTATIC" PARENT_SCOPE)
set(${lib_name}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(${lib_name}_LIBRARIES ${${lib_name}_LOCATION} PARENT_SCOPE)
