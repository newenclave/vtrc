#include <iostream>

#include <thread>
#include "boost/thread.hpp"
#include "boost/bind.hpp"

template <typename T> T *& get_ptr( )
{
    static __thread T * t = nullptr;
    return t;
}

template<typename T>
class thread_local_ptr {

public:

    typedef T value_type;
    typedef T * ptr_type;

    ptr_type &get( )
    {
        return get_ptr<T>( );
    }

};

typedef thread_local_ptr<int> thread_ptr_int;

void get_set_ptr( int i, thread_ptr_int &data )
{
    if( data.get( ) != nullptr ) {
        std::cerr << "NOT EMPTY!\n";
        std::terminate( );
    }
    data.get( ) = new int( i );
    for( int j=0; j<1000; ++j ) {
        (*data.get( )) = j;
        int m = (*data.get( ));

        if( j != m ) {
            std::cerr << "Failed!\n";
        }
    }
    delete data.get( );
}

int main( )
{
    boost::thread_group tg;
    thread_ptr_int ti;

    ti.get( ) = new int( 100 );

    std::cout << "Ti for main thread: " << (*ti.get( )) << std::endl;

    for( int i=0; i<100; ++i ) {
        tg.create_thread( boost::bind( get_set_ptr, i, boost::ref( ti ) ) );
    }

    tg.join_all( );
    delete ti.get( );

    return 0;
}

