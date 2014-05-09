    Remote filesystem example
===============

Operation support: 

        exists: Checks if path exists
    
        file_size: returns size of remote file
    
        info: returns information about remote FS object (is_exist, is_directory,
    	is_empty, is_regular, is_symlink)
    
        mkdir: for making remote directory
    
        del: for removing remote directory if it is empty
    
        Remote directory iteration is also support:
            iter_begin: starts iteration
	    iter_next: move iterator to next object ( boost::filesystem::directory_iterator.operator ++ )
	    iter_info: returns info about iteator FS object (similar to info)
	    iter_clone: returns new iterator object

    remote file operation:
	
