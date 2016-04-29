#ifndef VTRC_CONNECTION_IFACE_H
#define VTRC_CONNECTION_IFACE_H

#include "vtrc-memory.h"
#include "vtrc-function.h"
#include "vtrc-closure.h"

namespace boost {

    namespace system {
        class error_code;
    }

    namespace asio {
        class io_service;
    }
}

namespace vtrc { namespace rpc {
    class lowlevel_unit;
} }

namespace vtrc { namespace common {

    class enviroment;
    class protocol_layer;
    class call_context;
    class rpc_channel;

    struct native_handle_type {
        typedef void *pvoid;
        typedef pvoid handle;
        union {
#ifdef _WIN32
            handle  win_handle;
#else
            int     unix_fd;
#endif
            handle  ptr_;
        } value;
    };

    struct connection_iface: public enable_shared_from_this<connection_iface> {

        virtual ~connection_iface( ) { }

        virtual       std::string name( ) const         = 0;
        virtual const std::string &id( )  const         = 0;

        virtual void close( )                           = 0;
        virtual bool active( ) const                    = 0;
        virtual common::enviroment   &get_enviroment( ) = 0;
        virtual const call_context   *get_call_context( ) const = 0;

        vtrc::weak_ptr<connection_iface> weak_from_this( )
        {
            return vtrc::weak_ptr<connection_iface>( shared_from_this( ) );
        }

        vtrc::weak_ptr<connection_iface const> weak_from_this( ) const
        {
            return vtrc::weak_ptr<connection_iface const>( shared_from_this( ));
        }

        virtual native_handle_type native_handle( ) const = 0;

        /// llu is IN OUT parameter
        /// dont do this if not sure!
        virtual void raw_call_local ( vtrc::shared_ptr<rpc::lowlevel_unit> llu,
                                      common::empty_closure_type done ) = 0;

        virtual void raw_write ( vtrc::shared_ptr<rpc::lowlevel_unit> llu ) = 0;

        virtual void drop_service( const std::string &name ) = 0;
        virtual void drop_all_services(  ) = 0;

    protected:

        friend class rpc_channel;
        friend class call_keeper;

        virtual protocol_layer &get_protocol( )   = 0;

    };

    typedef vtrc::shared_ptr<connection_iface> connection_iface_sptr;
    typedef vtrc::weak_ptr<connection_iface>   connection_iface_wptr;

}}

#endif // VTRCCONNECTIONIFACE_H
