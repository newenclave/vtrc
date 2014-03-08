
set(lib_name vtrc_common_s)

include_directories( ${PROJECT_SOURCE_DIR} )

list(APPEND src_dirs
        .
        ./hash
        ./proto-helper
        ./config
    )

foreach( src_dir ${src_dirs} )
    aux_source_directory(${src_dir} lib_src)
    file(GLOB headers ${src_dir}/*.h)
    list(APPEND lib_src ${headers})
endforeach( )

set( CONFIG_FILES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/config)
include_directories( ${CONFIG_FILES_DIRECTORY} )

####################### PROTOCOL BASE #######################

file(GLOB protofiles protocol-base/*.proto)

set( GENERATED_FILES_DIR ${CMAKE_CURRENT_BINARY_DIR} )
set( GENERATED_FILES_OUTPUT ${GENERATED_FILES_DIR}/protocol )
file(MAKE_DIRECTORY ${GENERATED_FILES_OUTPUT})

include_directories( ${GENERATED_FILES_DIR} )

foreach(next_proto ${protofiles})

    get_filename_component(out_FILE ${next_proto} NAME_WE)
    get_filename_component(in_PATH  ${next_proto} PATH)

    set( OUTPUT_FILES
                ${GENERATED_FILES_OUTPUT}/${out_FILE}.pb.h
                ${GENERATED_FILES_OUTPUT}/${out_FILE}.pb.cc
       )

    add_custom_command(
        COMMAND
                ${PROTOBUF_PROTOC_EXECUTABLE}
                        --proto_path=${PROTOBUF_INCLUDE_DIRS}
                        --proto_path=${POTOBUF_IMPORT_DIRS}
                        --proto_path=${in_PATH}
                        --cpp_out=${GENERATED_FILES_OUTPUT}
                        ${next_proto}
        WORKING_DIRECTORY
                ${GENERATED_FILES_OUTPUT}
        OUTPUT
                ${OUTPUT_FILES}
        DEPENDS
                ${next_proto}
        )

    set_source_files_properties(
        ${OUTPUT_FILES}
        PROPERTIES
        GENERATED TRUE
    )

    list( APPEND lib_src ${OUTPUT_FILES})
    list( APPEND lib_src ${next_proto})

endforeach()

##################################################

add_library(${lib_name} STATIC ${lib_src})

list(APPEND dependencies
    Boost
    PROTOBUF
    )

message( "-- Seting up dependencies for " ${lib_name} )
foreach( dep ${dependencies} )

    message( "\tAdd dependence: " ${dep} )

    add_dependencies(       ${lib_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    include_directories(    ${${dep}_INCLUDE_DIRS}  )
    target_link_libraries(  ${lib_name} ${${dep}_LIBRARIES} )

endforeach(dep)

set_target_properties(${lib_name} PROPERTIES COMPILE_DEFINITIONS "STATIC")
get_target_property(${lib_name}_LOCATION ${lib_name} LOCATION)

set(${lib_name}_DEFINITIONS "-DSTATIC" PARENT_SCOPE)

set(${lib_name}_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GENERATED_FILES_DIR}
    ${CONFIG_FILES_DIRECTORY}
    PARENT_SCOPE
)

set(${lib_name}_LIBRARIES ${${lib_name}_LOCATION} PARENT_SCOPE)
