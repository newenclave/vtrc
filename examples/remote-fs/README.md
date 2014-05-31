
===============

Operations supported ([protocol definition file](https://github.com/newenclave/vtrc/blob/master/examples/remote-fs/protocol/remotefs.proto "remotefs.proto")): 

    exists:	Checks if the path exists
    
    file_size:	Returns size of the remote file
    
    info:	Returns information about remote FS object;
                is_exist, is_directory, is_empty, is_regular, is_symlink
    
    mkdir:	For making remote directory
    
    del:	For removing remote directory if it is empty
    
Remote directory iteration is also supported:

    iter_begin: Starts iteration

    iter_next:  Moves iterator to next object ( boost::filesystem::directory_iterator::operator ++ )

    iter_info:  Returns info about iteator FS object (similar to info)

    iter_clone: Returns new iterator object

Remote file operations:
        
    open:       Opens remote file. fopen-style open mode is supported
                ("rb", "wb", "a", ... etc; for more detailed information, read 'man fopen' )

    close:      For close file

    tell:       Obtains the current value of the remote file position

    seek:       Sets the remote file position
    
    read:       Reads block of data from the remote file; 
                In example blocks from 1 to 640000 bytes are supported
                Returns the number of bytes read or 0 is end of the remote file  was reached

    write:      Writes portion of data.
                Returns the number of bytes written.

Example usage
=====================

Server side:
-------
Command line options:
    
'--server' (or '-s') sets endpoint for server-side application. 
The library supports 2 transport families: TCP and Local system transport 
(UNIX sockets for UNIXes and Pipes for Windows); 

Example: 

open 2 endpoints tcp for 44555 and unix socket for /home/sandbox/fs.sock

    remote_fs_server -s 0.0.0.0:44555 -s /home/sandbox/fs.sock

tcp6 is also supported; 

    remote_fs_server -s :::44556  

or for windows; opens tcp6 endpoint for [::]:44556,
tcp endpoint for 192.168.1.101:10100 and windows pipe endpoint for pipe\\fs_local_pipe

    remote_fs_server -s :::44556 -s 192.168.1.101:10100 -s \\\\.\\pipe\\fs_local_pipe

For C++: ( function 'create_from_string' in [server.cpp](https://github.com/newenclave/vtrc/blob/master/examples/remote-fs/server/server.cpp#L95 "GITHUB file server.cpp") )

```cpp
std::vector<std::string> params;    // contains local_name (size( )==1) 
                                    // or tcp address and port (size( ) == 2)
......
if( params.size( ) == 1 ) {         /// local endpoint

    result = server::listeners::local::create( app, params[0] );

} else if( params.size( ) == 2 ) {  /// TCP

    result = server::listeners::tcp::create( app,
            params[0],
            boost::lexical_cast<unsigned short>(params[1]));

}
```

and (function 'start' in [server.cpp](https://github.com/newenclave/vtrc/blob/master/examples/remote-fs/server/server.cpp#L168 "GITHUB file server.cpp")  )
	    
```cpp
std::vector<std::string> servers; /// contains all '-s' params from command line
........  
for( const auto &s : servers  ) {
    std::cout << "Starting listener at '" << s << "'...";
    server::listener_sptr new_listener = create_from_string( s, app );
    listeners.push_back(new_listener);
    app.attach_listener( new_listener );
    new_listener->start( );
    std::cout << "Ok\n";
}
```

Client side:
-----------

Client can be used for access to the remote filesystem. There are several commands:
    
    Client help:
    ./remote_fs_client --help
   
    Allowed options:
    -? [ --help ]           help message
    -s [ --server ] arg     server name; <tcp address>:<port> or <pipe/file name>
    -p [ --path ] arg       Init remote path for client
    -w [ --pwd ]            Show current remote path
    -i [ --info ] arg       Show info about remote path
    -l [ --list ]           list remote directory
    -t [ --tree ]           show remote directory as tree
    -g [ --pull ] arg       Download remote file
    -u [ --push ] arg       Upload local file
    -G [ --pull-tree ] arg  Download remote fs tree
    -U [ --push-tree ] arg  Upload local fs tree
    -o [ --output ] arg     Target path; for pull-tree it means local directory
    -b [ --block-size ] arg Block size for pull and push; 1-65000
    -m [ --mkdir ] arg      Create remote directory
    -d [ --del ] arg        Remove remote directory if it is empty
    
option '--server' or '-s' has the same format as shown early: 

    -s 127.0.0.1:55555 

or 

    --server=/home/sandbox/fs.sock 

or 

    -s \\\\.\\pipe\\remote_fs_pipe
    
Client uses this option as a server point.

'--path' is init directory for client. Server will use it as start point for relative paths. 
Every client' command can use an absolute or relative paths. For example:
    
    remote_fs_client -s 192.168.1.100:55555 --path=/home/sandbox --mkdir=test 

will make /home/sandbox/test directory

    remote_fs_client -s 192.168.1.100:55555 --mkdir=/home/sandbox/test 

will make the same



