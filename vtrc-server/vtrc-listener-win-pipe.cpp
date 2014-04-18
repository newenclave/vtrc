#include "vtrc-listener-win-pipe.h"

#ifdef _WIN32

#include "boost/asio.hpp"
#include "boost/asio/windows/stream_handle.hpp"

#include "vtrc-application.h"
#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-common/vtrc-transport-win-pipe.h"
#include "vtrc-common/vtrc-connection-list.h"

#include "vtrc-connection-impl.h"

#include "vtrc-atomic.h"
#include "vtrc-common/vtrc-closure.h"

namespace vtrc { namespace server { namespace listeners {

namespace {

    namespace basio = boost::asio;
    typedef basio::windows::stream_handle    socket_type;
    typedef common::transport_win_pipe       connection_type;
    typedef connection_impl<connection_type> connection_impl_type;

    typedef vtrc::function<
        void (const common::connection_iface *)
    > close_closure;

    struct commection_pipe_impl: public connection_impl_type {

        typedef commection_pipe_impl this_type;

        commection_pipe_impl( listener &listen,
                              vtrc::shared_ptr<socket_type> sock,
                              const close_closure &on_destroy)
            :connection_impl_type(listen, sock, on_destroy)
        { }

        bool impersonate( )
        {
//            BOOL imp = ImpersonateNamedPipeClient(
//                                    get_socket( ).native_handle( ) );
//            if( imp ) {
//                common::call_context *c
//                                    (get_protocol(  ).get_top_call_context( ));
//                if( c ) c->set_impersonated( true );
//            }
//            return !!imp;
            return false;
        }

        void revert( )
        {
//            common::call_context *c
//                                (get_protocol( ).get_top_call_context( ));
//            if( c && c->get_impersonated( ) ) RevertToSelf( );
        }

        static vtrc::shared_ptr<this_type> create(listener &listnr,
                                 vtrc::shared_ptr<socket_type> sock,
                                 const close_closure &on_destroy)
        {
            vtrc::shared_ptr<this_type> new_inst
                    (vtrc::make_shared<this_type>(vtrc::ref(listnr),
                                                   sock, on_destroy));

            new_inst->init( );
            return new_inst;
        }
    };

    typedef commection_pipe_impl transport_type;

    struct pipe_listener: public listener {

        typedef pipe_listener this_type;
        typedef vtrc::shared_ptr< vtrc::atomic<size_t> > shared_counter_type;

        basio::io_service       &ios_;

        std::string              endpoint_;
        size_t                   pipe_max_inst_;
        size_t                   in_buf_size_;
        size_t                   out_buf_size_;

        basio::windows::overlapped_ptr overlapped_;
        bool                     working_;

        pipe_listener( application &app,
                const listener_options &opts, const std::string pipe_name,
                size_t max_inst)
            :listener(app, opts)
            ,ios_(app.get_io_service( ))
            ,endpoint_(pipe_name)
            ,pipe_max_inst_(max_inst)
            ,in_buf_size_(opts.read_buffer_size)
            ,out_buf_size_(opts.read_buffer_size)
            ,working_(true)
        { }

        virtual ~pipe_listener( ) { }

        void on_client_destroy( vtrc::weak_ptr<listener> inst,
                                const common::connection_iface *conn )
        {
            vtrc::shared_ptr<listener> lock( inst.lock( ) );
            if( lock ) {
                this->stop_connection( conn );
            }
        }

        close_closure get_on_destroy( )
        {
            return vtrc::bind( &this_type::on_client_destroy, this,
                               weak_from_this( ), _1 );
        }

        std::string string( ) const
        {
            return std::string("pipe://") + endpoint_;
        }

        void start_accept(  )
        {
            if( !working_ ) return;
            SECURITY_DESCRIPTOR secdesc;
            SECURITY_ATTRIBUTES secarttr;
            InitializeSecurityDescriptor(&secdesc,
                                         SECURITY_DESCRIPTOR_REVISION);

            SetSecurityDescriptorDacl(&secdesc, TRUE,
                                      static_cast<PACL>(NULL), FALSE);

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
                            basio::placeholders::error, new_sock) );

                BOOL res = ConnectNamedPipe( pipe_hdl, overlapped_.get( ) );

                DWORD last_error(GetLastError());
                if( res || ( last_error ==  ERROR_IO_PENDING ) ) {
                    overlapped_.release();

                } else if( last_error == ERROR_PIPE_CONNECTED ) {
                    bsys::error_code ec( 0,
                            basio::error::get_system_category( ));
                    overlapped_.complete( ec, 0 );

                } else {
                    bsys::error_code ec(last_error,
                                basio::error::get_system_category( ));
                    overlapped_.complete( ec, 0 );

                }
            } else {
                bsys::error_code ec(GetLastError( ),
                            basio::error::get_system_category( ));
                overlapped_.complete( ec, 0 );
            }
        }

        void start( )
        {
            start_accept( );
            on_start_( );
        }

        void stop ( )
        {
            working_ = false;
            bsys::error_code ec( ERROR_CANCELLED,
                        basio::error::get_system_category( ));
            overlapped_.complete( ec, 0 );
            on_stop_( );
        }

        void on_accept( const bsys::error_code &error,
                        vtrc::shared_ptr<socket_type> sock )
        {
            if( !error ) {
                try {
                    vtrc::shared_ptr<transport_type> new_conn
                             (transport_type::create(
                                *this, sock, get_on_destroy( )));
                    get_application( ).get_clients( )->store( new_conn );
                    new_connection( new_conn.get( ) );

                } catch( ... ) {
                    ;;;
                }
                start_accept( );
            } else {
                // delete sock;
            }
        }

    };
}

    namespace win_pipe {
        listener_sptr create( application &app, const listener_options &opts,
                                const std::string &name )
        {
            vtrc::shared_ptr<pipe_listener>new_l
                    (vtrc::make_shared<pipe_listener>(
                         vtrc::ref(app), vtrc::ref(opts),
                         vtrc::ref(name),
                         PIPE_UNLIMITED_INSTANCES));
            app.attach_listener( new_l );
            return new_l;
        }

        listener_sptr create( application &app, const std::string &name )
        {
            listener_options def_opts(default_options( ));
            return create( app, def_opts, name );
        }

        //endpoint_iface *create( application &app, const std::wstring &name )
        //{
        //
        //}
    }

}}}

#endif
