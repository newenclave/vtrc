#include "protocol/remotefs.pb.h"
#include "protocol/vtrc-errors.pb.h"

#include "boost/filesystem.hpp"

#include "vtrc-common/vtrc-connection-iface.h"

#include "google/protobuf/descriptor.h"
#include "vtrc-common/vtrc-closure-holder.h"

#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-common/vtrc-exception.h"

#include "vtrc-atomic.h"

namespace {

    using namespace vtrc;
    namespace gpb = google::protobuf;
    namespace fs  = boost::filesystem;

    typedef std::map<gpb::uint32, fs::path>               path_map;
    typedef std::map<gpb::uint32, fs::directory_iterator> iterator_map;
    typedef std::map<gpb::uint32, FILE *>                 files_map;

    class remote_fs_impl: public vtrc_example::remote_fs {

        vtrc::common::connection_iface *connection_;
        vtrc::atomic<gpb::uint32>     handle_index_;

        path_map            fs_inst_;
        vtrc::shared_mutex  fs_inst_lock_;

        iterator_map        iterators_;
        vtrc::shared_mutex  iterators_lock_;

        gpb::uint32 next_index( )
        {
            return ++handle_index_;
        }

        fs::path path_from_request( const vtrc_example::fs_handle_path* request,
                                    gpb::uint32 &hdl)
        {
            fs::path p(request->path( ));

            if( !request->has_handle( ) || p.is_absolute( ) ) {
                /// ok. new instance requested
                p.normalize( );
                hdl = next_index( );

            } else {
                /// old path must be used
                hdl = request->handle( ).value( );
                vtrc::upgradable_lock l( fs_inst_lock_ );
                path_map::const_iterator f(fs_inst_.find( hdl ));

                if( f == fs_inst_.end( ) )
                    throw vtrc::common::exception(
                            vtrc_errors::ERR_NOT_FOUND, "Bad handle" );

                p = f->second;
                p /= request->path( );
                p.normalize( );
            }
            return p;
        }

        /// close fs instance or iterator
        ///
        void close(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle* request,
             ::vtrc_example::fs_handle* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            {
                vtrc::upgradable_lock ul( fs_inst_lock_ );
                path_map::iterator f( fs_inst_.find( request->value( ) ) );
                if( f != fs_inst_.end( ) ) {
                    vtrc::upgrade_to_unique uul( ul );
                    fs_inst_.erase( f );
                    response->set_value( request->value( ) );
                    return;
                }
            }

            {
                vtrc::upgradable_lock ul( iterators_lock_ );
                iterator_map::iterator f( iterators_.find( request->value( )));
                if( f != iterators_.end( ) ) {
                    vtrc::upgrade_to_unique uul( ul );
                    iterators_.erase( f );
                    response->set_value( request->value( ) );
                }
            }
        }

        void open(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle_path* request,
             ::vtrc_example::fs_handle_path* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            gpb::uint32 hdl;
            fs::path p(path_from_request( request, hdl ));

            vtrc::unique_shared_lock l( fs_inst_lock_ );
            fs_inst_.insert( std::make_pair( hdl, p ) );

            response->mutable_handle( )->set_value( hdl );
            response->set_path( p.string( ) );
        }

        void cd(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle_path* request,
             ::vtrc_example::fs_handle_path* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
        }

        void pwd(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle_path* request,
             ::vtrc_example::fs_handle_path* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );

            gpb::uint32 hdl = request->handle( ).value( );

            vtrc::upgradable_lock l( fs_inst_lock_ );
            path_map::const_iterator f(fs_inst_.find( hdl ));

            if( f == fs_inst_.end( ) )
                throw vtrc::common::exception(
                        vtrc_errors::ERR_NOT_FOUND, "Bad handle" );
            response->set_path( f->second.string( ) );
        }

        void exists(::google::protobuf::RpcController* controller,
                     const ::vtrc_example::fs_handle_path* request,
                     ::vtrc_example::fs_element_info* response,
                     ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            gpb::uint32 hdl;
            fs::path p(path_from_request( request, hdl ));
            response->set_is_exist( fs::exists( p ) );
        }

        void info(::google::protobuf::RpcController* controller,
                     const ::vtrc_example::fs_handle_path* request,
                     ::vtrc_example::fs_element_info* response,
                     ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            gpb::uint32 hdl;
            fs::path p(path_from_request( request, hdl ));

            bool is_exists = fs::exists( p );
            response->set_is_exist( is_exists );
            if( is_exists ) {
                bool is_dir = fs::is_directory( p );
                response->set_is_directory( is_dir );
                if( is_dir )
                    response->set_is_empty(fs::is_empty( p ));
                response->set_is_regular( fs::is_regular_file( p ) );
            }

        }


        void begin(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle_path* request,
             ::vtrc_example::fs_iterator_info* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
        }

        void next(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_iterator_info* request,
             ::vtrc_example::fs_iterator_info* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
        }

    public:
        remote_fs_impl(vtrc::common::connection_iface *connection)
            :connection_(connection)
            ,handle_index_(100)
        { }
    };
}

namespace remote_fs {
    google::protobuf::Service *create(vtrc::common::connection_iface *client )
    {
        return new remote_fs_impl( client );
    }
    std::string name( )
    {
        return vtrc_example::remote_fs::descriptor( )->full_name( );
    }
}

namespace remote_file {

    google::protobuf::Service *create(vtrc::common::connection_iface *client )
    {
        return NULL;
    }

    std::string name( )
    {
        return vtrc_example::remote_file::descriptor( )->full_name( );
    }
}
