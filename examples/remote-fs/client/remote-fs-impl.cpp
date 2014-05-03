#include "remote-fs-iface.h"
#include "vtrc-client-base/vtrc-rpc-channel-c.h"
#include "vtrc-client-base/vtrc-client.h"
#include "protocol/remotefs.pb.h"

namespace {

    namespace gpb = google::protobuf;

    struct remote_fs_impl: public interfaces::remote_fs {
        typedef remote_fs_impl this_type;

        typedef vtrc_example::remote_fs_Stub stub_type;

        vtrc::shared_ptr<google::protobuf::RpcChannel> channel_;
        mutable stub_type                              stub_;
        gpb::uint32                                    fs_handle_;

        ~remote_fs_impl( ) try
        {
            if( fs_handle_ ) {
                vtrc_example::fs_handle h;
                h.set_value( fs_handle_ );
                stub_.close( NULL, &h, &h, NULL );
            }

        } catch ( ... ) {
            ;;;
        }

        remote_fs_impl( vtrc::shared_ptr<vtrc::client::vtrc_client> client,
                        std::string const &path)
            :channel_(client->create_channel( ))
            ,stub_(channel_.get( ))
            ,fs_handle_(0)
        {
            vtrc_example::fs_handle_path hp;
            hp.set_path( path );
            stub_.open( NULL, &hp, &hp, NULL );
            fs_handle_ = hp.handle( ).value( );
        }

        void cd( const std::string &new_path ) const
        {
            vtrc_example::fs_handle_path hp;
            hp.set_path( new_path );
            hp.mutable_handle( )->set_value( fs_handle_ );
            stub_.cd( NULL, &hp, &hp, NULL );
        }

        unsigned get_handle( ) const
        {
            return fs_handle_;
        }
    };

}

namespace interfaces {

    remote_fs *create_remote_fs(
                vtrc::shared_ptr<vtrc::client::vtrc_client> client,
                std::string const &path )
    {
        return new remote_fs_impl( client, path );
    }

}

