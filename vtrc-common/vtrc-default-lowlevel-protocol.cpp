#include <iostream>
#include <algorithm>

#include "vtrc-default-lowlevel-protocol.h"
#include "vtrc-common/vtrc-protocol-accessor-iface.h"

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

#include "vtrc-common/vtrc-protocol-defaults.h"

#include "protocol/vtrc-rpc-options.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include "boost/system/error_code.hpp"

namespace vtrc { namespace common { namespace lowlevel {

    namespace gpb  = google::protobuf;
    namespace spns = data_queue::varint;

    std::string protocol_layer_iface::serialize_message( const message_type *m )
    {
        return m->SerializeAsString( );
    }

    void protocol_layer_iface::parse_message( const std::string &data,
                                              message_type *mess )
    {
        mess->ParseFromString( data );
    }


    default_protocol::default_protocol( )
        :hash_maker_(common::hash::create_default( ))
        ,hash_checker_(common::hash::create_default( ))
        ,transformer_(common::transformers::none::create( ))
        ,revertor_(common::transformers::none::create( ))
        ,pa_(NULL)

    {
        rpc::session_options opts = defaults::session_options( );
        queue_.reset( spns::create_parser(opts.max_message_length( ) ) );
    }

    default_protocol::~default_protocol( )
    {
        //std::cout << "Destroyed!\n";
    }

    std::string default_protocol::serialize_message(
            const protocol_layer_iface::message_type *mess)
    {
        return mess->SerializeAsString( );
    }

    void default_protocol::parse_message( const std::string &data,
                                protocol_layer_iface::message_type *mess)
    {
        mess->ParseFromString( data );
    }

    void default_protocol::set_transformer( transformer_iface *ti )
    {
        transformer_.reset( ti );
    }

    void default_protocol::set_revertor( transformer_iface *ti )
    {
        revertor_.reset( ti );
    }

    void default_protocol::set_hash_maker( hash_iface *hi )
    {
        hash_maker_.reset( hi );
    }

    void default_protocol::set_hash_checker( hash_iface *hi )
    {
        hash_checker_.reset( hi );
    }

    void default_protocol::configure(const rpc::session_options &opts )
    {
        queue_->set_maximum_length( opts.max_message_length( ) );
        configure_impl( opts );
    }

    protocol_accessor *default_protocol::accessor( )
    {
        return pa_;
    }

    void default_protocol::set_accessor( protocol_accessor *pa )
    {
        pa_ = pa;
    }

    void default_protocol::configure_impl( const rpc::session_options & )
    { }

    std::string
    default_protocol::pack_message( const rpc::lowlevel_unit &mess )
    {
//        return mess.SerializeAsString( );
        std::string ser(mess.SerializeAsString( ));
        return pack_message( ser.c_str( ), ser.size( ) );
    }

    std::string default_protocol::pack_message( const std::string &data )
    {
        return pack_message( data.c_str( ), data.size( ) );
    }

    std::string default_protocol::pack_message( const char *data, size_t len )
    {
        /**
         * message_header = <packed_size(data_length + hash_length)>
        **/
        const size_t body_len = len + hash_maker_->hash_size( );
        std::string result( spns::pack_size( body_len ));

        /** here is:
         *  message_body = <hash(data)> + <data>
        **/
        std::string body( hash_maker_->get_data_hash( data, len ) );
        body.append( data, data + len );

        /**
         * message =  message_header + <transform( message )>
        **/
        transformer_->transform( body );
//            transformer_->transform( body.empty( ) ? NULL : &body[0],
//                                     body.size( ) );

        result.append( body.begin( ), body.end( ) );

        return result;
    }

    void default_protocol::process_data( const char *data, size_t len )
    {
        if( len > 0 ) {

            //std::string next_data(data, data + length);

            /**
             * message = <size>transformed(data)
             * we must revert data in 'parse_and_pop'
            **/

            //queue_->append( &next_data[0], next_data.size( ));

            size_t old = queue_->messages( ).size( );
            queue_->append( data, len );
            queue_->process( );
            if( old < queue_->messages( ).size( ) ) {
                accessor( )->message_ready( );
            }
        }
    }

    size_t default_protocol::queue_size( ) const
    {
        return queue_->messages( ).size( );
    }

    bool default_protocol::check_message( const std::string &mess )
    {
        const size_t hash_length = hash_checker_->hash_size( );
        const size_t diff_len    = mess.size( ) - hash_length;

        bool result = false;

        if( mess.size( ) >= hash_length ) {
            result = hash_checker_->
                     check_data_hash( mess.c_str( ) + hash_length,
                                      diff_len,
                                      mess.c_str( ) );
        }
        return result;
    }

    bool default_protocol::pop_proto_message( rpc::lowlevel_unit &res )
    {
        std::string &data(queue_->messages( ).front( ));

        /// revert message
        revertor_->transform( data );
//            revertor_->transform( data.empty( ) ? NULL : &data[0],
//                                  data.size( ) );
        /// check hash
        bool checked = check_message( data );
        if( checked ) {
            /// parse
            checked = parse_raw_message( data, res );
        }

        /// in all cases we pop message
        queue_->messages( ).pop_front( );
        return checked;
    }

    bool default_protocol::pop_raw_message( std::string &result )
    {
        std::string &data( queue_->messages( ).front( ) );

        /// revert message
        revertor_->transform( data );
//        std::cout << "Pop: " << data << "\n";
//            revertor_->transform( data.empty( ) ? NULL : &data[0],
//                                  data.size( ) );

        /// check hash
        bool checked = check_message( data );
        //std::cout << "Checked: " << checked << "\n";
        if( checked ) {
            const size_t hash_length = hash_checker_->hash_size( );
            result.assign( data.c_str( ) + hash_length,
                           data.size( )  - hash_length );
        }

        /// in all cases we pop message
        queue_->messages( ).pop_front( );

        return checked;
    }

    bool default_protocol::parse_raw_message(const std::string &mess,
                                            google::protobuf::Message &out )
    {
        const size_t hash_length = hash_checker_->hash_size( );
        return out.ParseFromArray( mess.c_str( ) + hash_length,
                                   mess.size( )  - hash_length );
    }

    void default_protocol::init( protocol_accessor *pa, system_closure_type cb )
    {
        set_accessor( pa );
        pa->ready( true );
        cb( VTRC_SYSTEM::error_code( ) );
    }

    void default_protocol::close( )
    {
        ;;;
    }

    void default_protocol::do_handshake( )
    {
        ;;;
    }

    bool default_protocol::ready( ) const
    {
        return true;
    }

    namespace dumb {
        protocol_layer_iface *create( )
        {
            return new default_protocol( );
        }
    }

}}}

#if 0
std::string prepare_data( const char *data, size_t length )
{
    /**
     * message_header = <packed_size(data_length + hash_length)>
    **/
    const size_t body_len = length + hash_maker_->hash_size( );

    std::string body( body_len + 32, '?' );

    /**
     *  Pack length of the data and the hash to vector
     *  Get size of packed size value;
    **/
    size_t siz_len = size_policy_ns::pack_size_to( body_len, &body[0] );

    /**
     *  here is:
     *  message_body = <hash(data)> + <data>
    **/
    hash_maker_->get_data_hash( data, length, &body[siz_len] );

    if( length > 0 ) {
        memcpy( &body[hash_maker_->hash_size( ) + siz_len],
                 data, length );
    }

    /**
     *  transform( <size><hash><message> )
    **/
    body.resize( body_len + siz_len );

    transformer_->transform( body );

    return body;
}

void process_data( const char *data, size_t length )
{
    if( length > 0 ) {

        std::string next_data(data, data + length);

        const size_t old_size = queue_->messages( ).size( );

        /// revert data block
        revertor_->transform( next_data );

        /**
         * message = transformed(<size>data)
         * we must revert data here
        **/

        //queue_->append( &next_data[0], next_data.size( ));

        queue_->append( next_data.empty( ) ? NULL : &next_data[0],
                        next_data.size( ) );
        queue_->process( );

        if( queue_->messages( ).size( ) > old_size ) {
            parent_->on_data_ready( );
        }
    }
}

bool raw_pop( std::string &result )
{
    /// doesntwork
    std::string &data( queue_->messages( ).front( ) );

    /// revert message
    revertor_->transform( data );
//            revertor_->transform( data.empty( ) ? NULL : &data[0],
//                                  data.size( ) );

    /// check hash
    bool checked = check_message( data );
    if( checked ) {
        const size_t hash_length = hash_checker_->hash_size( );
        result.assign( data.c_str( ) + hash_length,
                       data.size( )  - hash_length );
    }

    /// in all cases we pop message
    queue_->messages( ).pop_front( );

    return checked;
}

bool parse_and_pop( gpb::MessageLite &result )
{
    std::string &data(queue_->messages( ).front( ));

    /// check hash
    bool checked = check_message( data );
    if( checked ) {
        /// parse
        checked = parse_message( data, result );
    }

    /// in all cases we pop message
    queue_->messages( ).pop_front( );
    return checked;
}
#endif
