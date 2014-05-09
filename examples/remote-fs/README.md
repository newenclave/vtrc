Remote filesystem example
===============

Operations supported: 

    exists:	Checks if the path exists
    
    file_size:	Returns size of the remote file
    
    info:	Returns information about remote FS object;
                is_exist, is_directory, is_empty, is_regular, is_symlink
    
    mkdir:	Ror making remote directory
    
    del:	For removing remote directory if it is empty
    
Remote directory iteration is also supported:

    iter_begin: Starts iteration

    iter_next:  Move iterator to next object ( boost::filesystem::directory_iterator.operator ++ )

    iter_info:  Returns info about iteator FS object (similar to info)

    iter_clone: Returns new iterator object

Remote file operations:
        
    open:       Opens remote file. fopen-style open mode is supported
                ("rb", "wb", "a", ... etc; read man fopen )

    close:      For close file

    tell:       Obtains the current value of the remote file position

    seek:       Sets the remote file position
    
    read:       Reads block of data from the remote file; 
                In example blocks from 1 to 640000 bytes arte supported
                Returns the number of bytes read or 0 is end of the remote file  was reached

    write:      Writes portion of data.
                Returns the number of bytes written.

