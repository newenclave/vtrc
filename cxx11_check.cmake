macro( check_cxx11 VAR_ENABLED )

#-std=c++11 by default

set( ${VAR_ENABLED} 1 )

    if( MSVC )                                          ### MS

        if( NOT ( MSVC10 OR MSVC11 OR MSVC12 ) )
            set( ${VAR_ENABLED} 0 )
        endif( )

    elseif( CMAKE_COMPILER_IS_GNUCXX )                  ### CNU

        if( ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.6 )
            set( ${VAR_ENABLED} 0 )
        endif( )

    elseif( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )    ### Clang

        if( ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 3.1 )
            set( ${VAR_ENABLED} 0 )
        endif( )

    endif( )

endmacro( )


macro( get_cmd_cxx11 COMMAND_VAR )

    if( MSVC )                                          ### MS

        set( ${COMMAND_VAR} "" )

    elseif( CMAKE_COMPILER_IS_GNUCXX )                  ### CNU

        set( ${COMMAND_VAR} "-std=c++11" )

    elseif( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )    ### Clang

        set( ${COMMAND_VAR} "-std=c++11" )

    endif( )

endmacro( )
