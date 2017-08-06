
#ifdef _WIN32

#include "vtrc-asio.h"

#include "vtrc/server/application.h"
#include "vtrc/common/environment.h"
#include "vtrc/common/exception.h"
#include "vtrc/common/connection-list.h"

#include "vtrc/common/transport-win-pipe.h"

#include "vtrc/common/protocol-accessor-iface.h"
#include "vtrc/common/protocol/vtrc-rpc-lowlevel.pb.h"
#include "vtrc/common/closure.h"

#include "vtrc/server/connection-impl.h"

namespace vtrc { namespace server { namespace listeners {

namespace {

    namespace basio = VTRC_ASIO;
    typedef basio::windows::stream_handle    socket_type;
    typedef common::transport_win_pipe       connection_type;
    typedef connection_impl<connection_type> connection_impl_type;

    typedef vtrc::server::connection_close_closure close_closure;

    struct commection_pipe_impl: public connection_impl_type {

        typedef commection_pipe_impl this_type;
        std::string name_;

        commection_pipe_impl( listener &listen,
                              vtrc::shared_ptr<socket_type> sock,
                              const close_closure &on_close_cb,
                              const std::string &name )
            :connection_impl_type(listen, sock, on_close_cb)
            ,name_(name)
        { }

        std::string name(  ) const
        {
            return name_;
        }

        bool is_local( ) const
        {
            return true;
        }

        static vtrc::shared_ptr<this_type> create( listener &listnr,
                                 vtrc::shared_ptr<socket_type> sock,
                                 const close_closure &on_close_cb,
                                 const std::string &name )
        {
            vtrc::shared_ptr<this_type> new_inst
                    (vtrc::make_shared<this_type>(vtrc::ref(listnr),
                                        sock,
                                        vtrc::ref(on_close_cb),
                                        vtrc::ref(name)));

            rpc::session_options const &opts(listnr.get_options( ));

            new_inst->protocol_.reset(
                    new protocol_layer_s( new_inst->app_,
                                            new_inst.get( ), opts ) );

            //new_inst->protocol_ ->init( );
            //new_inst->init( );
            return new_inst;
        }
    };

    typedef commection_pipe_impl transport_type;

    struct pipe_listener: public listener {

        typedef pipe_listener this_type;

        basio::io_service       &ios_;

        std::string              endpoint_;
        size_t                   pipe_max_inst_;
        size_t                   in_buf_size_;
        size_t                   out_buf_size_;

        basio::windows::overlapped_ptr overlapped_;
        bool                           working_;

        pipe_listener( application &app,
                const rpc::session_options &opts,
                const std::string &pipe_name,
                size_t max_inst)
            :listener(app, opts)
            ,ios_(app.get_io_service( ))
            ,endpoint_(pipe_name)
            ,pipe_max_inst_(max_inst)
            ,in_buf_size_(opts.read_buffer_size( ))
            ,out_buf_size_(opts.read_buffer_size( ))
            ,working_(false)
        { }

        virtual ~pipe_listener( ) { }

        void on_client_destroy( vtrc::weak_ptr<listener> inst,
                                common::connection_iface *conn )
        {
            vtrc::shared_ptr<listener> lock( inst.lock( ) );
            if( lock ) {
                this->stop_connection( conn );
            }
        }

        close_closure get_on_close_cb( )
        {
            return vtrc::bind( &this_type::on_client_destroy, this,
                                weak_from_this( ),
                                vtrc::placeholders::_1 );
        }

        std::string name( ) const
        {
            return std::string("pipe://") + endpoint_;
        }

        bool is_local( ) const
        {
            return true;
        }

        void start_accept( bool throw_if_fail )
        {
            if( !working_ ) return;

            SECURITY_DESCRIPTOR secdesc;
            SECURITY_ATTRIBUTES secarttr;
            InitializeSecurityDescriptor( &secdesc,
                                          SECURITY_DESCRIPTOR_REVISION );

            SetSecurityDescriptorDacl( &secdesc, TRUE,
                                       static_cast<PACL>(NULL), FALSE );

            secarttr.nLength              = sizeof(SECURITY_ATTRIBUTES);
            secarttr.bInheritHandle       = FALSE;
            secarttr.lpSecurityDescriptor = &secdesc;

            HANDLE pipe_hdl = CreateNamedPipeA( endpoint_.c_str( ),
                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                PIPE_TYPE_BYTE     | PIPE_READMODE_BYTE,
                pipe_max_inst_,
                out_buf_size_, in_buf_size_,
                0, &secarttr );

            if( INVALID_HANDLE_VALUE != pipe_hdl ) {

                vtrc::shared_ptr<socket_type> new_sock
                        (vtrc::make_shared<socket_type>(vtrc::ref(ios_)));
                new_sock->assign( pipe_hdl );

                overlapped_.reset( ios_,
                        vtrc::bind( &this_type::on_accept, this,
                            vtrc::placeholders::error, new_sock,
                            weak_from_this( ) ) );

                BOOL res = ConnectNamedPipe( pipe_hdl, overlapped_.get( ) );

                DWORD last_error( GetLastError( ) );

                if( res || ( last_error ==  ERROR_IO_PENDING ) ) {

                    overlapped_.release();

                } else if( last_error == ERROR_PIPE_CONNECTED ) {

                    bsys::error_code ec( 0,
                            basio::error::get_system_category( ));
                    overlapped_.complete( ec, 0 );

                } else {

                    bsys::error_code ec(last_error,
                                basio::error::get_system_category( ));

                    if( throw_if_fail ) {
                        vtrc::common::raise( bsys::system_error( ec ) );
                        return;
                    } else {
                        overlapped_.complete( ec, 0 );
                    }

                }
            } else {
                bsys::error_code ec( GetLastError( ),
                            basio::error::get_system_category( ));
                if( throw_if_fail ) {
                    vtrc::common::raise( bsys::system_error( ec ) );
                    return;
                } else {
                    overlapped_.complete( ec, 0 );
                }
            }
        }

        void start( )
        {
            working_ = true;
            start_accept( true );
            call_on_start( );
        }

        void stop ( )
        {
            working_ = false;
            bsys::error_code ec( ERROR_CANCELLED,
                        basio::error::get_system_category( ));
            overlapped_.complete( ec, 0 );
            call_on_stop( );
        }

        bool is_active( ) const
        {
            return working_;
        }

        void on_accept( const bsys::error_code &error,
                        vtrc::shared_ptr<socket_type> sock,
                        vtrc::weak_ptr<listener> &inst)
        {
            vtrc::shared_ptr<listener> locked( inst.lock( ));
            if( !locked ) {
                return;
            }

            if( !error ) {
                try {
                    vtrc::shared_ptr<transport_type> new_conn
                             (transport_type::create(
                                *this, sock, get_on_close_cb( ),
                                name( )));

                    namespace ph = vtrc::placeholders;

                    new_conn->get_protocol( )
                             .assign_lowlevel_factory(
                                lowlevel_protocol_factory( ) );

                    get_application( ).get_clients( )->store( new_conn );
                    new_connection( new_conn.get( ) );

                    //new_conn->init( );

                    get_application( ).get_io_service( )
                        .dispatch( vtrc::bind( &connection_type::init,
                                                new_conn ) );


                } catch( ... ) {
                    ;;;
                }
                start_accept( false );
            } else {
                if( working_ ) {
                    stop( );
                    call_on_accept_failed( error );
                } else {
                    call_on_stop( );
                }
            }
        }

    };
}

    namespace win_pipe {
        listener_sptr create( application &app,
                              const rpc::session_options &opts,
                              const std::string &name )
        {
            vtrc::shared_ptr<pipe_listener>new_l
                    (vtrc::make_shared<pipe_listener>(
                         vtrc::ref(app), vtrc::ref(opts),
                         vtrc::ref(name),
                         PIPE_UNLIMITED_INSTANCES));
            return new_l;
        }

        listener_sptr create( application &app, const std::string &name )
        {
            const rpc::session_options
                    def_opts( common::defaults::session_options( ) );

            return create( app, def_opts, name );
        }

        //endpoint_iface *create( application &app, const std::wstring &name )
        //{
        //
        //}
    }

}}}

#endif
