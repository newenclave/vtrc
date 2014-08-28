
if(MSVC)

    foreach(flag_var         
        CMAKE_CXX_FLAGS_DEBUG_INIT 
        CMAKE_CXX_FLAGS_RELEASE_INIT
        CMAKE_CXX_FLAGS_MINSIZEREL_INIT 
        CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT
        CMAKE_C_FLAGS_DEBUG_INIT 
        CMAKE_C_FLAGS_RELEASE_INIT
        CMAKE_C_FLAGS_MINSIZEREL_INIT 
        CMAKE_C_FLAGS_RELWITHDEBINFO_INIT)
        
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        
    endforeach(flag_var)

endif(MSVC)
