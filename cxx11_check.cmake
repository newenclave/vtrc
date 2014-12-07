macro( check_cxx11 VAR_ENABLED )

    #-std=c++11 by default
    set( ${VAR_ENABLED} 1 )

    if( MSVC )                                          ### MS

        if( NOT ( MSVC10 OR MSVC11 OR MSVC12 ) )
            set( ${VAR_ENABLED} 0 )
        endif( )

    elseif( CMAKE_COMPILER_IS_GNUCXX )                  ### GNU

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

    elseif( CMAKE_COMPILER_IS_GNUCXX )                  ### GNU

        set( ${COMMAND_VAR} "-std=c++11 " )

    elseif( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )    ### Clang

        set( ${COMMAND_VAR} "-std=c++11" )

    endif( )

endmacro( )

macro( get_compiler_thread_local MACRO_VAR )

    if( MSVC )                                          ### MS

        set( ${MACRO_VAR} __declspec\(thread\) )

    elseif( CMAKE_COMPILER_IS_GNUCXX )                  ### GNU

        if( ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.8 )
            set( ${MACRO_VAR} __thread )
        else( )
            set( ${MACRO_VAR} thread_local )
        endif( )

    elseif( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )    ### Clang

        set( ${MACRO_VAR} thread_local )

    endif( )

endmacro( )
