#include "protocol/remotefs.pb.h"

#include "boost/filesystem.hpp"

#include "vtrc-common/vtrc-connection-iface.h"

#include "google/protobuf/descriptor.h"
#include "vtrc-common/vtrc-closure-holder.h"

#include "vtrc-common/vtrc-mutex-typedefs.h"

namespace {

    using namespace vtrc;
    namespace gpb = google::protobuf;
    namespace fs  = boost::filesystem;

    typedef std::map<gpb::uint32, fs::path>               path_map;
    typedef std::map<gpb::uint32, fs::directory_iterator> iterator_map;
    typedef std::map<gpb::uint32, FILE *>                 files_map;

    class remote_fs_impl: public vtrc_example::remote_fs {

        vtrc::common::connection_iface *connection_;
        gpb::uint32     handle_index_;

        path_map        fs_inst_;
        iterator_map    iterators_;

        void open(::google::protobuf::RpcController* controller,
             const ::vtrc_example::fs_handle_path* request,
             ::vtrc_example::fs_handle_path* response,
             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );
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
    const std::string name( )
    {
        return vtrc_example::remote_fs::descriptor( )->full_name( );
    }
}

namespace remote_file {

    google::protobuf::Service *create(vtrc::common::connection_iface *client )
    {
        return NULL;
    }

    const std::string name( )
    {
        return vtrc_example::remote_file::descriptor( )->full_name( );
    }
}
