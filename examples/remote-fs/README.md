    Remote filesystem example
===============

Operation support: 

        exists:	    Checks if path exists
    
        file_size:  Returns size of remote file
    
        info:	    Returns information about remote FS object (is_exist, is_directory,
                    is_empty, is_regular, is_symlink)
    
        mkdir:	    Ror making remote directory
    
        del:	    For removing remote directory if it is empty
    
Remote directory iteration is also support:

        iter_begin: Starts iteration

        iter_next:  Move iterator to next object ( boost::filesystem::directory_iterator.operator ++ )

        iter_info:  Returns info about iteator FS object (similar to info)

        iter_clone: Returns new iterator object

Remote file operation:
	
