
set( lib_name ${VTRC_COMMON_LIB_NAME} )

list(APPEND src_dirs
        .
        ./hash
        ./proto-helper
        ./crypt/chacha
        ./transport
        ${VTRC_INCLUDE_DIR}/vtrc/common
        ${VTRC_INCLUDE_DIR}/vtrc/common/config
        ${VTRC_INCLUDE_DIR}/vtrc/common/observer
        ${VTRC_INCLUDE_DIR}/vtrc/common/details
        ${VTRC_INCLUDE_DIR}/vtrc/common/observer/traits
    )

foreach( src_dir ${src_dirs} )
    aux_source_directory(${src_dir} lib_src)
    file(GLOB headers ${src_dir}/*.h)
    list(APPEND lib_src ${headers})
endforeach( )

set( CONFIG_FILES_DIRECTORY ${VTRC_INCLUDE_DIR}/vtrc/common/config )

include_directories( ${CONFIG_FILES_DIRECTORY}  )

####################### PROTOCOL BASE #######################

set( ${lib_name}_POTOBUF_IMPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protocol-base )

file(GLOB protofiles protocol-base/*.proto)

set( ${lib_name}_PROTOBUF_FILES_DIR    ${CMAKE_CURRENT_BINARY_DIR} )

set( ${lib_name}_PROTOBUF_FILES_OUTPUT
     ${${lib_name}_PROTOBUF_FILES_DIR} )

file( MAKE_DIRECTORY ${${lib_name}_PROTOBUF_FILES_OUTPUT})

include_directories( ${${lib_name}_PROTOBUF_FILES_DIR} )

foreach(next_proto ${protofiles})

    get_filename_component( out_FILE ${next_proto} NAME_WE )
    get_filename_component( in_PATH  ${next_proto} PATH )

    set( OUTPUT_FILES
             ${${lib_name}_PROTOBUF_FILES_OUTPUT}/${out_FILE}.pb.h
             ${${lib_name}_PROTOBUF_FILES_OUTPUT}/${out_FILE}.pb.cc
             ${VTRC_PROTOCOL_INCLUDE_DIR}/${out_FILE}.pb.h
       )

    add_custom_command(
        COMMAND
                ${PROTOBUF_PROTOC_EXECUTABLE}
        ARGS
                --proto_path=${PROTOBUF_INCLUDE_DIRS}
                --proto_path=${POTOBUF_IMPORT_DIRS}
                --proto_path=${in_PATH}
                --cpp_out=${${lib_name}_PROTOBUF_FILES_OUTPUT}
                ${next_proto}

        COMMAND
                ${CMAKE_COMMAND} -E copy
                    ${${lib_name}_PROTOBUF_FILES_OUTPUT}/${out_FILE}.pb.h
                    ${VTRC_PROTOCOL_INCLUDE_DIR}/${out_FILE}.pb.h

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

endforeach( )

list( APPEND public_proto_files
        vtrc-rpc-options.proto
        #vtrc-errors.proto
    )

foreach( next_proto ${public_proto_files} )

    file( COPY
              protocol-base/${next_proto}
          DESTINATION
              ${VTRC_PROTOCOL_INCLUDE_DIR} )

endforeach( )

##################################################


add_library(${lib_name} ${VTRC_LINK_TYPE} ${lib_src})

if( OPENSSL_FOUND )
    list( APPEND dependencies  OPENSSL )
endif( OPENSSL_FOUND )

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
    #target_link_libraries(  ${lib_name} ${${dep}_LIBRARIES} )

endforeach(dep)

set_target_properties( ${lib_name}
                       PROPERTIES
                       COMPILE_DEFINITIONS "${VTRC_LINK_TYPE}")

get_target_property(${lib_name}_LOCATION ${lib_name} LOCATION)

set(${lib_name}_DEFINITIONS "-D${VTRC_LINK_TYPE}" PARENT_SCOPE)

set( ${lib_name}_PROTOBUF_FILES_OUTPUT
     ${${lib_name}_PROTOBUF_FILES_OUTPUT}
     PARENT_SCOPE
)

set( ${lib_name}_PROJECT_SOURCE_DIR
     ${PROJECT_SOURCE_DIR}
       PARENT_SCOPE
)

set( ${lib_name}_POTOBUF_IMPORT_DIR
     ${${lib_name}_POTOBUF_IMPORT_DIR}
     PARENT_SCOPE
)

set( ${lib_name}_INCLUDE_DIRS
     ${CMAKE_CURRENT_SOURCE_DIR}
     ${${lib_name}_PROTOBUF_FILES_DIR}
     #${${lib_name}_PROTOBUF_FILES_OUTPUT}
     ${CONFIG_FILES_DIRECTORY}
     ${Boost_INCLUDE_DIRS}
     ${PROTOBUF_INCLUDE_DIRS}
     ${OPENSSL_INCLUDE_DIRS}
     PARENT_SCOPE
)

set( ${lib_name}_LIBRARY ${${lib_name}_LOCATION} PARENT_SCOPE)

set( ${lib_name}_LIBRARIES
     ${${lib_name}_LOCATION}
     ${Boost_LIBRARIES}
     ${PROTOBUF_LIBRARIES}
     ${OPENSSL_LIBRARIES}
     PARENT_SCOPE)

