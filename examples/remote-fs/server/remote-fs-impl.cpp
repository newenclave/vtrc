#include <stdio.h>
#include <errno.h>

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

    typedef vtrc::shared_ptr<FILE> file_ptr;

    typedef std::map<gpb::uint32, fs::path>               path_map;
    typedef std::map<gpb::uint32, fs::directory_iterator> iterator_map;
    typedef std::map<gpb::uint32, file_ptr >              files_map;

    class remote_file_impl: public vtrc_example::remote_file {

        vtrc::common::connection_iface *connection_;
        vtrc::atomic<gpb::uint32>       handle_index_;

        files_map           files_;
        vtrc::shared_mutex  files_lock_;

        gpb::uint32 next_index( )
        {
            return ++handle_index_;
        }

        file_ptr file_from_hdl( gpb::uint32 hdl )
        {
            vtrc::shared_lock sl( files_lock_ );
            files_map::iterator f(files_.find( hdl ));
            if( f == files_.end( ) ) {
                throw vtrc::common::exception(
                      vtrc_errors::ERR_NOT_FOUND, "Bad file handle" );
            }
            return f->second;
        }

        void open(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::file_open_req* request,
                 ::vtrc_example::fs_handle* response,
                 ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);

            std::string path(request->path( ));
            std::string mode(request->mode( ));

            FILE *f = fopen( path.c_str( ), mode.c_str( ) );

            if( !f ) {
                throw vtrc::common::exception(
                            errno, vtrc_errors::CATEGORY_SYSTEM,
                            "fopen failed");
            }

            file_ptr fnew( f, fclose );

            gpb::uint32 hdl(next_index( ));
            vtrc::unique_shared_lock usl( files_lock_ );
            files_.insert( std::make_pair( hdl, fnew ) );
            response->set_value( hdl );
        }

        void close(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::fs_handle* request,
                 ::vtrc_example::fs_handle* response,
                 ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);
            vtrc::unique_shared_lock usl( files_lock_ );
            files_.erase( request->value( ) );
        }

        void tell(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::fs_handle* request,
                 ::vtrc_example::file_position* response,
                 ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);
            file_ptr f(file_from_hdl( request->value( ) ));
            long pos = ftell( f.get( ) );
            if( -1 == pos ) {
                throw vtrc::common::exception(
                            errno, vtrc_errors::CATEGORY_SYSTEM,
                            "ftell failed");
            }
            response->set_position( static_cast<gpb::uint64>(pos) );
        }

        void seek(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::file_set_position* request,
                 ::vtrc_example::file_position* response,
                 ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);

            file_ptr f(file_from_hdl( request->hdl( ).value( ) ));

            int pos = fseek( f.get( ), request->position( ),
                                        request->whence( ) );
            if( -1 == pos ) {
                throw vtrc::common::exception(
                            errno, vtrc_errors::CATEGORY_SYSTEM,
                            "fseek failed");
            }
            response->set_position( request->position( ) );
        }

        void read(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::file_data_block* request,
                 ::vtrc_example::file_data_block* response,
                 ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);

            size_t len = request->length( ) > (44 * 1024)
                    ? (44 * 1024)
                    : request->length( );

            if( !len ) {
                response->set_length( 0 );
                return;
            }
            file_ptr f(file_from_hdl(request->hdl( ).value( )));
            std::vector<char> data(len);
            size_t r = fread( &data[0], 1, len, f.get( ) );
            response->set_length( r );
            response->set_data( &data[0], r );
        }

        void write(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::file_data_block* request,
                 ::vtrc_example::file_data_block* response,
                 ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);
            file_ptr f(file_from_hdl(request->hdl( ).value( )));
            size_t w = fwrite( request->data( ).c_str( ), 1,
                                request->data( ).size( ), f.get( ) );
            response->set_length( w );
        }

    public:

        remote_file_impl(vtrc::common::connection_iface *connection)
            :connection_(connection)
            ,handle_index_(100)
        { }

    };

    class remote_fs_impl: public vtrc_example::remote_fs {

        vtrc::common::connection_iface *connection_;
        vtrc::atomic<gpb::uint32>       handle_index_;

        path_map            fs_inst_;
        vtrc::shared_mutex  fs_inst_lock_;

        iterator_map        iterators_;
        vtrc::shared_mutex  iterators_lock_;

        gpb::uint32 next_index( )
        {
            return ++handle_index_;
        }

        fs::directory_iterator iter_from_request( gpb::uint32 &hdl)
        {
            vtrc::shared_lock l( iterators_lock_ );
            iterator_map::iterator f(iterators_.find(hdl));
            if( f == iterators_.end( ) ) {
                throw vtrc::common::exception(
                        vtrc_errors::ERR_NOT_FOUND, "Bad iterator handle" );
            }
            return f->second;
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

                vtrc::shared_lock l( fs_inst_lock_ );
                path_map::const_iterator f(fs_inst_.find( hdl ));

                if( f == fs_inst_.end( ) ) {
                    throw vtrc::common::exception(
                            vtrc_errors::ERR_NOT_FOUND, "Bad fs handle" );
                }

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

            vtrc::upgradable_lock ul( fs_inst_lock_ );

            gpb::uint32 hdl(request->handle( ).value( ));

            path_map::iterator f(fs_inst_.find( hdl ));

            if( f == fs_inst_.end( ) ) {
                throw vtrc::common::exception(
                        vtrc_errors::ERR_NOT_FOUND, "Bad fs handle" );
            }

            fs::path p(f->second / request->path( ));
            p.normalize( );

            /// set new path
            vtrc::upgrade_to_unique utul( ul );
            f->second = p;
        }

        void pwd(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle_path* request,
             ::vtrc_example::fs_handle_path* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );

            gpb::uint32 hdl;
            fs::path p(path_from_request( request, hdl ));

            response->set_path( p.string( ) );
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

        void file_size(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle_path* request,
             ::vtrc_example::file_position* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            gpb::uint32 hdl;
            fs::path p(path_from_request( request, hdl ));
            response->set_position(fs::file_size( p ));
        }


        void fill_info( fs::path const &p,
                        vtrc_example::fs_element_info* response )
        {
            bool is_exists = fs::exists( p );
            response->set_is_exist( is_exists );
            if( is_exists ) {
                bool is_dir = fs::is_directory( p );
                response->set_is_symlink( fs::is_symlink( p ) );
                response->set_is_directory( is_dir );
                if( is_dir )
                    response->set_is_empty(fs::is_empty( p ));
                else
                    response->set_is_empty(true);
                response->set_is_regular( fs::is_regular_file( p ) );
            }
        }

        void fill_iter_info( const fs::directory_iterator &iter,
                             gpb::uint32 hdl,
                             vtrc_example::fs_iterator_info* response)
        {
            response->mutable_handle( )->set_value( hdl );
            response->set_end( iter == fs::directory_iterator( ));
            if( !response->end( ) )
                response->set_path( iter->path( ).string( ) );
        }

        void info(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle_path* request,
             ::vtrc_example::fs_element_info* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            gpb::uint32 hdl;
            fs::path p(path_from_request( request, hdl ));
            fill_info( p, response );
        }

        void mkdir(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle_path* request,
             ::vtrc_example::fs_handle_path* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            gpb::uint32 hdl(0);
            fs::path p(path_from_request( request, hdl ));
            fs::create_directories( p );
            response->mutable_handle( )->set_value( hdl );
        }

        void del(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle_path* request,
             ::vtrc_example::fs_handle_path* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            gpb::uint32 hdl(0);
            fs::path p(path_from_request( request, hdl ));
            fs::remove_all( p );
            response->mutable_handle( )->set_value( hdl );
        }

        void iter_begin(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle_path* request,
             ::vtrc_example::fs_iterator_info* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            gpb::uint32 hdl;
            fs::path p(path_from_request( request, hdl ));
            fs::directory_iterator new_iterator(p);
            gpb::uint32 iter_hdl = next_index( );
            vtrc::unique_shared_lock usl(iterators_lock_);
            iterators_.insert( std::make_pair( iter_hdl, new_iterator ) );
            fill_iter_info(new_iterator, iter_hdl, response);
        }

        void iter_next(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_iterator_info* request,
             ::vtrc_example::fs_iterator_info* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            gpb::uint32 hdl(request->handle( ).value( ));
            fs::directory_iterator iter(iter_from_request( hdl ));
            ++iter;
            fill_iter_info(iter, hdl, response);
            vtrc::unique_shared_lock usl( iterators_lock_ );
            iterators_[hdl] = iter;
        }

        void iter_info(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_iterator_info* request,
             ::vtrc_example::fs_element_info* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            gpb::uint32 hdl(request->handle( ).value( ));
            fs::directory_iterator iter(iter_from_request( hdl ));
            fill_info( iter->path( ), response );
        }

        void iter_clone(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_iterator_info* request,
             ::vtrc_example::fs_iterator_info* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
            gpb::uint32 hdl(request->handle( ).value( ));
            fs::directory_iterator iter(iter_from_request( hdl ));
            gpb::uint32 new_hdl = next_index( );
            fill_iter_info( iter, hdl, response );
            vtrc::unique_shared_lock usl(iterators_lock_);
            iterators_.insert( std::make_pair( new_hdl, iter ) );
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
        return new remote_file_impl( client );
    }

    std::string name( )
    {
        return vtrc_example::remote_file::descriptor( )->full_name( );
    }
}
