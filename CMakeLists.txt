
cmake_minimum_required(VERSION 2.8)

project(vtrc)



add_subdirectory(vtrc-common)
add_subdirectory(vtrc-server)

if( BUILD_TESTS ) 
	add_subdirectory(vtrc-test-server)
endif( )
