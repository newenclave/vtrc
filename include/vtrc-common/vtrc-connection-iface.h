#ifndef VTRC_CONNECTION_IFACE_H
#define VTRC_CONNECTION_IFACE_H

#include "vtrc-memory.h"
#include "vtrc-function.h"
#include "vtrc-closure.h"

#include "vtrc-asio-forward.h"
#include "vtrc-system-forward.h"
#include "vtrc-lowlevel-protocol-iface.h"

#include "vtrc-handle.h"

VTRC_ASIO_FORWARD( class io_service; )
VTRC_SYSTEM_FORWARD( class error_code; )

namespace vtrc { namespace rpc {
    class lowlevel_unit;
} }

namespace vtrc { namespace common {

    class  enviroment;
    class  call_context;
    class  rpc_channel;
    struct protocol_iface;

    struct connection_iface: public enable_shared_from_this<connection_iface> {

        connection_iface( )
            :user_data_(NULL)
        { }

        virtual ~connection_iface( ) { }

        virtual       std::string name( ) const         = 0;
        virtual const std::string &id( )  const         = 0;

        virtual void init( )                            = 0;
        virtual void close( )                           = 0;
        virtual bool active( ) const                    = 0;

        //virtual common::enviroment   &get_enviroment( ) = 0;
        //virtual const call_context   *get_call_context( ) const = 0;

        vtrc::weak_ptr<connection_iface> weak_from_this( )
        {
            return vtrc::weak_ptr<connection_iface>( shared_from_this( ) );
        }

        vtrc::weak_ptr<connection_iface const> weak_from_this( ) const
        {
            return vtrc::weak_ptr<connection_iface const>( shared_from_this( ));
        }

        void drop_service( const std::string &name );
        void drop_all_services(  );
        void raw_write ( vtrc::shared_ptr<rpc::lowlevel_unit> llu );

        const lowlevel::protocol_layer_iface *get_lowlevel( ) const;

        virtual native_handle_type native_handle( ) const = 0;

        /// llu is IN OUT parameter
        /// dont do this if not sure!
        virtual void raw_call_local ( vtrc::shared_ptr<rpc::lowlevel_unit> llu,
                                      common::empty_closure_type done )
        { }

        virtual void write( const char *data, size_t length ) = 0;
        virtual void write( const char *data, size_t length,
                            const system_closure_type &success,
                            bool success_on_send ) = 0;

        virtual protocol_iface &get_protocol( ) = 0;
        virtual const protocol_iface &get_protocol( ) const = 0;

        void *user_data( )
        {
            return user_data_;
        }

        const void *user_data( ) const
        {
            return user_data_;
        }

        void set_user_data( void *ud )
        {
            user_data_ = ud;
        }

    private:
        void *user_data_;

    };

    class connection_empty: public connection_iface {

        common::protocol_iface *protocol_;

        common::protocol_iface &get_protocol( )
        {
            return *protocol_;
        }

        const common::protocol_iface &get_protocol( ) const
        {
            return *protocol_;
        }

    protected:

        connection_empty( )
            :protocol_(NULL)
        { }

    public:

        ~connection_empty( );

        void init( )
        { }

        void set_protocol( common::protocol_iface* protocol )
        {
            protocol_ = protocol;
        }

        virtual std::string name( ) const;

        virtual const std::string &id( ) const
        {
            static const std::string id( "" );
            return id;
        }

        virtual void close( )
        { }

        virtual bool active( ) const
        {
            return true;
        }

        virtual common::native_handle_type native_handle( ) const
        {
            common::native_handle_type res;
            res.value.ptr_ = NULL;
            return res;
        }

    };

    typedef vtrc::shared_ptr<connection_iface> connection_iface_sptr;
    typedef vtrc::weak_ptr<connection_iface>   connection_iface_wptr;

}}

#endif // VTRCCONNECTIONIFACE_H
