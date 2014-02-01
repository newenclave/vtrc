#include <iostream>
#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <stdlib.h>

#include "vtrc-server/vtrc-application-iface.h"
#include "vtrc-server/vtrc-endpoint-iface.h"
#include "vtrc-server/vtrc-connection-iface.h"
#include "vtrc-server/vtrc-endpoint-tcp.h"

#include "vtrc-common/vtrc-thread-poll.h"
#include "vtrc-common/vtrc-sizepack-policy.h"

#include "vtrc-common/vtrc-hasher-iface.h"
#include "vtrc-common/vtrc-hasher-impls.h"

#include "protocol/vtrc-errors.pb.h"

namespace ba = boost::asio;

class main_app: public vtrc::server::application_iface
{
    ba::io_service ios_;

    typedef
    std::map <
        vtrc::server::connection_iface *,
        boost::shared_ptr<vtrc::server::connection_iface>
    > connection_map_type;

    connection_map_type connections_;
    boost::shared_mutex connections_lock_;

    typedef boost::unique_lock<boost::shared_mutex> unique_lock;
    typedef boost::shared_lock<boost::shared_mutex> shared_lock;

public:

    main_app( )
    {}

    ba::io_service &get_io_service( )
    {
        return ios_;
    }

private:

    void on_endpoint_started( vtrc::server::endpoint_iface *ep )
    {
        std::cout << "Start endpoint: " << ep->string( ) << "\n";
    }

    void on_endpoint_stopped( vtrc::server::endpoint_iface *ep )
    {
        std::cout << "Stop endpoint: " << ep->string( ) << "\n";
    }

    void on_endpoint_exception( vtrc::server::endpoint_iface *ep )
    {
        try {
            throw;
        } catch( const std::exception &ex ) {
            std::cout << "Endpoint exception '" << ep->string( )
                      << "': " << ex.what( ) << "\n";
        } catch( ... ) {
            throw;
        }
    }

    void on_new_connection_accepted(
                    vtrc::server::connection_iface* connection )
    {
        unique_lock l( connections_lock_ );
        connections_.insert( std::make_pair(connection,
              boost::shared_ptr<vtrc::server::connection_iface>(connection) ) );
        std::cout << "connection accepted\n";
    }

    void on_new_connection_ready(
                            vtrc::server::connection_iface* connection )
    {
        std::cout << "connection ready\n";
    }

    void on_connection_die( vtrc::server::connection_iface* connection )
    {
        unique_lock l( connections_lock_ );
        connections_.erase( connection );
        std::cout << "connection die\n";
    }

};

class data_collector {

protected:

    std::deque<char>        data_;
    std::deque<std::string> packed_;

public:

    virtual ~data_collector( ) { }

    void append( const char *data, size_t length )
    {
        data_.insert( data_.end( ), data, data + length );
    }

    size_t data_size( ) const
    {
        return data_.size( );
    }

    size_t size( ) const
    {
        return packed_.size( );
    }

    void grab_all( std::deque<std::string> &messages )
    {
        std::deque<std::string> tmp;
        packed_.swap( tmp );
        messages.swap( tmp );
    }

    std::string &front( )
    {
        packed_.front( );
    }

    void pop_front( )
    {
        packed_.pop_front( );
    }

    virtual void process( ) = 0;
};

template <typename SizePackPolicy>
class data_packer: public data_collector {
public:
    void process( )
    {
        std::string new_data( SizePackPolicy::pack( data_.size( ) ) );
        new_data.insert( new_data.end( ), data_.begin( ), data_.end( ) );
        packed_.push_back( new_data );
        data_.clear( );
    }
};

template <typename SizePackPolicy>
class data_unpacker: public data_collector {

    const size_t maximum_valid_length;

public:

    data_unpacker( size_t maximum_valid )
        :maximum_valid_length(maximum_valid)
    {}

    void process( )
    {
        typedef SizePackPolicy SPP;
        size_t next;
        bool valid_data = true;
        while( valid_data &&
             ( next = SPP::size_length(data_.begin( ), data_.end( ))) )
        {

            if( next > SPP::max_length )
                throw std::length_error( "The serialized data is invalid" );

            size_t len = SPP::unpack(data_.begin( ), data_.end( ));

            if( len > maximum_valid_length )
                throw std::length_error( "Message is too long" );

            if( (len + next) <= data_.size( ) ) {

                std::deque<char>::iterator b( data_.begin( ) );
                std::deque<char>::iterator n( b );
                std::deque<char>::iterator e( b );

                std::advance( n, next );
                std::advance( e, len + next );

                std::string mess( n, e );
                packed_.push_back( mess );
                data_.erase( b, e );

            } else {
                valid_data = false;
            }
        }

    }
};


int main( )
{

    data_packer< vtrc::common::policies::varint_policy<size_t> > collector;
    data_unpacker< vtrc::common::policies::varint_policy<size_t> > unpacker(1000);

    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );
    collector.append( "123456796", 9 );

    collector.process( );

    std::cout << collector.size( ) << " " << collector.data_size( ) << "\n";


    try {
        unpacker.append( collector.front( ).c_str(), collector.front( ).size( ));
        unpacker.process( );
        std::cout << unpacker.size( ) << ": " << unpacker.front( ).size( ) << "\n";

        std::cout << unpacker.front( ) << "\n";

    } catch( const std::exception &ex ) {
        std::cout << "error: " << ex.what( ) << "\n";
    }

    return 0;

    main_app app;
    vtrc::common::thread_poll poll(app.get_io_service( ), 4);

    boost::shared_ptr<vtrc::server::endpoint_iface> tcp_ep
            (vtrc::server::endpoints::tcp::create(app, "0.0.0.0", 44667));

    tcp_ep->start( );

    poll.join_all( );

    return 0;
}
