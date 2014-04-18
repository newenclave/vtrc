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

    struct commection_pipe_impl: public connection_impl_type {

        typedef commection_pipe_impl this_type;

        commection_pipe_impl( listener &listen,
                              vtrc::shared_ptr<socket_type> sock,
                              const common::empty_closure_type &on_destroy)
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
                                 const common::empty_closure_type &on_destroy)
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
        shared_counter_type      client_count_;

        basio::windows::overlapped_ptr overlapped_;

        pipe_listener( application &app,
                const listener_options &opts, const std::string pipe_name,
                size_t max_inst, size_t out_buf_size, size_t in_buf_size)
            :listener(app, opts)
            ,ios_(app.get_io_service( ))
            ,endpoint_(pipe_name)
            ,pipe_max_inst_(max_inst)
            ,in_buf_size_(in_buf_size)
            ,out_buf_size_(out_buf_size)
            ,client_count_(vtrc::make_shared<vtrc::atomic<size_t> >(0))
        { }

        virtual ~pipe_listener( ) { }

        static void on_client_destroy( shared_counter_type count )
        {
            --(*count);
        }

        common::empty_closure_type get_on_destroy( )
        {
            return vtrc::bind( &this_type::on_client_destroy, client_count_ );
        }

        size_t clients_count( ) const
        {
            return (*client_count_);
        }

        std::string string( ) const
        {
            return std::string("pipe://") + endpoint_;
        }

        void start_accept(  )
        {
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
            get_application( ).on_endpoint_started( this );
        }

        void stop ( )
        {
            get_application( ).on_endpoint_stopped( this );
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
                    ++(*client_count_);
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

        listener *create( application &app, const std::string &name )
        {
            listener_options def_opts(default_options( ));
            return new pipe_listener( app, default_options( ), name,
                                     PIPE_UNLIMITED_INSTANCES,
                                     def_opts.read_buffer_size,
                                     def_opts.read_buffer_size );
        }

        //endpoint_iface *create( application &app, const std::wstring &name )
        //{
        //
        //}

        listener *create( application &app, const listener_options &opts,
                                const std::string &name )
        {
            return new pipe_listener( app, opts, name,
                                     PIPE_UNLIMITED_INSTANCES,
                                     opts.read_buffer_size,
                                     opts.read_buffer_size );
        }
    }

}}}

#endif
