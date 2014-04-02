#include "vtrc-endpoint-win-pipe.h"

#ifdef _WIN32

#include <boost/asio.hpp>
#include <boost/asio/windows/stream_handle.hpp>

#include "vtrc-application.h"
#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-common/vtrc-transport-win-pipe.h"
#include "vtrc-common/vtrc-connection-list.h"

#include "vtrc-endpoint-iface.h"
#include "vtrc-connection-impl.h"

namespace vtrc { namespace server { namespace endpoints {

namespace {

    namespace basio = boost::asio;
    typedef basio::windows::stream_handle    socket_type;
    typedef common::transport_win_pipe       connection_type;
    typedef connection_impl<connection_type> connection_impl_type;

    struct commection_pipe_impl: public connection_impl_type {

        typedef commection_pipe_impl this_type;

        commection_pipe_impl( endpoint_iface &endpoint,
                            vtrc::shared_ptr<socket_type> sock )
            :connection_impl_type(endpoint, sock)
        { }

        bool impersonate( )
        {
            BOOL imp = ImpersonateNamedPipeClient(
                                    get_socket( ).native_handle( ) );
            return !!imp;
        }

        void revert( )
        {
            RevertToSelf( );
        }

        static vtrc::shared_ptr<this_type> create(endpoint_iface &endpoint,
                                                    socket_type *sock )
        {
            vtrc::shared_ptr<this_type> new_inst
                    (vtrc::make_shared<this_type>(vtrc::ref(endpoint),
                                    vtrc::shared_ptr<socket_type>(sock) ));

            new_inst->init( );
            return new_inst;
        }

    };

    typedef commection_pipe_impl transport_type;

    struct pipe_ep_impl: public endpoint_iface {

        typedef pipe_ep_impl this_type;

        application             &app_;
        basio::io_service       &ios_;
        common::enviroment       env_;

        endpoint_options         opts_;
        std::string              endpoint_;
        size_t                   pipe_max_inst_;

        basio::windows::overlapped_ptr ovl_;

        pipe_ep_impl( application &app,
                const endpoint_options &opts,
                const std::string pipe_name)
            :app_(app)
            ,ios_(app_.get_io_service( ))
            ,env_(app_.get_enviroment())
            ,opts_(opts)
            ,endpoint_(pipe_name)
            ,pipe_max_inst_(10)
        {}

        virtual ~pipe_ep_impl( ) { }

        application &get_application( )
        {
            return app_;
        }

        common::enviroment &get_enviroment( )
        {
            return env_;
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

            secarttr.nLength = sizeof(SECURITY_ATTRIBUTES);
            secarttr.bInheritHandle = FALSE;
            secarttr.lpSecurityDescriptor = &secdesc;

            HANDLE pipe_hdl = CreateNamedPipeA( endpoint_.c_str( ),
                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                pipe_max_inst_, 4096,
                4096, 0, &secarttr);

            if( INVALID_HANDLE_VALUE != pipe_hdl ) {

                socket_type *new_sock = new socket_type(ios_);

                //ba::windows::overlapped_ptr ovl;
                ovl_.reset( ios_,
                    vtrc::bind( &this_type::on_accept, this,
                        basio::placeholders::error, new_sock) );

                BOOL res = ConnectNamedPipe( pipe_hdl, ovl_.get( ) );

                new_sock->assign( pipe_hdl );

                DWORD last_error(GetLastError());
                if( res || ( last_error ==  ERROR_IO_PENDING ) ) {
                    ovl_.release();
                } else if( last_error == ERROR_PIPE_CONNECTED ) {
                    bsys::error_code ec(0,
                            basio::error::get_system_category( ));
                    ovl_.complete(ec, 0);
                } else {
                    bsys::error_code ec(GetLastError( ),
                                basio::error::get_system_category( ));
                    ovl_.complete(ec, 0);
                }
            } else {
                //std::cout << "Error on_pipe_connect\n";
            }
        }

        void start( )
        {
            start_accept( );
            app_.on_endpoint_started( this );
        }

        void stop ( )
        {
            app_.on_endpoint_stopped( this );
        }

        virtual const endpoint_options &get_options( ) const
        {
            return opts_;
        }

        void on_accept( const bsys::error_code &error,
                        socket_type* sock )
        {
            if( !error ) {
                try {
                    vtrc::shared_ptr<transport_type> new_conn
                             (transport_type::create( *this, sock ));
                    app_.get_clients( )->store( new_conn );
                } catch( ... ) {
                    ;;;
                }
                start_accept( );
            } else {
                delete sock;
            }
        }

    };
}

    namespace win_pipe {

        endpoint_options default_options( )
        {
            endpoint_options def_opts = { 5, 1024 * 1024, 20, 4096 };
            return def_opts;
        }


        endpoint_iface *create( application &app, const std::string &name )
        {
            return new pipe_ep_impl( app, default_options( ), name );
        }

        //endpoint_iface *create( application &app, const std::wstring &name )
        //{

        //}

        endpoint_iface *create( application &app, const endpoint_options &opts,
                                const std::string &name )
        {
            return new pipe_ep_impl( app, opts, name );
        }
    }

}}}

#endif
