#include <iostream>
#include "boost/asio.hpp"

#include <queue>
#include <set>

#include "boost/thread.hpp"

#include "common/async-transport-point.hpp"
#include "vtrc-signal-declaration.h"

namespace ba = boost::asio;

typedef ba::ip::tcp::socket stream_type;

typedef async_transport::point_iface<stream_type> async_point_type;

struct message_transformer {
    virtual std::string transform( std::string &data ) = 0;
};

struct write_transformer_none: public message_transformer {
    std::string transform( std::string &data )
    {
        return data;
    }
};

class my_async_reader: public async_point_type {

    typedef my_async_reader this_type;
    typedef boost::shared_ptr<message_transformer> transformer_sptr;
    transformer_sptr  transformer_;


protected:

    my_async_reader( ba::io_service &ios )
        :async_point_type(ios, 4096,
                          async_point_type::DONT_DISPATCH_READ,
                          async_point_type::DONT_TRANSFORM_MESSAGE)
        ,transformer_(new write_transformer_none)
    { }

public:

    static
    boost::shared_ptr<my_async_reader> create( ba::io_service &ios )
    {
        return  boost::shared_ptr<my_async_reader>( new my_async_reader(ios) );
    }

    void set_transformer( message_transformer *new_trans )
    {
        push_set_transformer( transformer_sptr(new_trans) );
    }

private:

    void push_set_transformer( transformer_sptr transform )
    {
        get_dispatcher( ).post(
                    boost::bind( &this_type::set_transformer_impl, this,
                                 transform, this->shared_from_this( ) ));
    }

    void set_transformer_impl( transformer_sptr transform,
                               async_point_type::shared_type /*inst*/)
    {
        transformer_ = transform;
    }

    VTRC_DECLARE_SIGNAL( on_read, void ( const char *, size_t ) );

    VTRC_DECLARE_SIGNAL( on_read_error,
                         void ( const boost::system::error_code & ) );

    VTRC_DECLARE_SIGNAL( on_write_error,
                         void ( const boost::system::error_code & ) );

    void on_read( const char *data, size_t length )
    {
        on_read_( data, length );
    }

    void on_read_error( const boost::system::error_code &code )
    {
        on_read_error_( code );
    }

    virtual void on_write_error( const boost::system::error_code &code )
    {
        on_write_error_( code );
    }

    std::string on_transform_message( std::string &message )
    {
        return transformer_->transform( message );
    }

};

typedef boost::shared_ptr<my_async_reader> stream_sptr;

std::set<stream_sptr> connections;

typedef boost::signals2::signal_type <
            void ( stream_sptr, const char *, size_t ),
            boost::signals2::keywords::mutex_type<boost::mutex>
        >::type read_signal_type;

read_signal_type g_read_signal;

void start_accept( ba::ip::tcp::acceptor &accept );

class my_transformer: public message_transformer
{
    const  std::string key_;
    size_t counter_;

public:

    my_transformer( const std::string &key )
        :key_(key)
        ,counter_(0)
    { }

private:
    std::string transform( std::string &data )
    {
        for( size_t i=0; i<data.size( ); ++i ) {
            ++counter_;
            counter_ %= key_.size( );
            data[i]  ^= key_[counter_];
        }
        return data;
    }
};

void on_error( stream_sptr ptr, const boost::system::error_code &err )
{
    std::cout << "Read error at "
              << ptr->get_stream( ).remote_endpoint( ) << ": "
              << err.message( )
              << "\n"
              ;
    ptr->close( );
    connections.erase( ptr );
    std::cout << "clients: " << connections.size( ) << "\n";
}

void on_client_read( stream_sptr ptr, const char *data, size_t length )
{
    std::cout << "got " << length << " bytes from "
              << ptr->get_stream( ).remote_endpoint( )
              << "\n";
    g_read_signal( ptr, data, length );
}

void post_write_message( const boost::system::error_code &error,
                         std::string data )
{
    if( !error ) {
        std::cout << "sent: " << data << "; No error\n";
    } else {
        std::cout << "sent: " << data
                  << "; failed: " << error.message( ) << "\n";
    }
}

void accept_handle( boost::system::error_code const &err,
                    stream_sptr stream,
                    ba::ip::tcp::acceptor &accept )
{
    if( !err ) {

        connections.insert( stream );

        stream->on_read_connect( boost::bind( on_client_read, stream, _1, _2 ));
        stream->on_read_error_connect( boost::bind( on_error, stream, _1 ));
        stream->set_transformer( new my_transformer( "123789" ) );
        stream->start_read( );

        g_read_signal.connect(
            read_signal_type::slot_type(
                    boost::bind( &my_async_reader::write, stream, _2, _3 )
            ).track( stream )
        );

        start_accept( accept );
        std::cout << "new point accepted: "
                  << stream->get_stream( ).remote_endpoint( )
                  << std::endl;
    } else {
        std::cout << "accept error\n";
    }
}

void start_accept( ba::ip::tcp::acceptor &accept )
{
    stream_sptr new_point(my_async_reader::create(accept.get_io_service( )));
    accept.async_accept(
                new_point->get_stream( ),
                boost::bind( accept_handle, ba::placeholders::error,
                             new_point, boost::ref( accept ) ) );
}

ba::ip::tcp::endpoint make_endpoint( const std::string &address,
                                     unsigned short port )
{
    return ba::ip::tcp::endpoint(ba::ip::address::from_string(address), port);
}

void run_ios( ba::io_service &ios )
{
    while( 1 ) {
        ios.run( );
    }
}

int main( ) try
{

    ba::io_service       ios;
    ba::io_service::work wrk(ios);

    ba::ip::tcp::acceptor acceptor( ios, make_endpoint("127.0.0.1", 55555) );

    start_accept( acceptor );

//    boost::thread t1( run_ios, boost::ref( ios ) );
//    boost::thread t2( run_ios, boost::ref( ios ) );
//    boost::thread t3( run_ios, boost::ref( ios ) );
//    boost::thread t4( run_ios, boost::ref( ios ) );

    run_ios( ios );

    return 0;

} catch( const std::exception &ex ) {

    std::cout << "main error: " << ex.what( ) << "\n";
    return 1;

}

