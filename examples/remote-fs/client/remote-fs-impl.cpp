#include "remote-fs-iface.h"
#include "vtrc-client-base/vtrc-rpc-channel-c.h"
#include "vtrc-client-base/vtrc-client.h"
#include "protocol/remotefs.pb.h"

namespace {

    struct remote_fs_impl: public interfaces::remote_fs {
        typedef remote_fs_impl this_type;

        typedef vtrc_example::remote_fs_Stub stub_type;

        vtrc::shared_ptr<google::protobuf::RpcChannel> channel_;
        mutable stub_type                              stub_;

        remote_fs_impl( vtrc::shared_ptr<vtrc::client::vtrc_client> client,
                        std::string const &path)
            :channel_(client->create_channel( ))
            ,stub_(channel_.get( ))
        {

        }

        void cd( const std::string &new_path ) const
        {

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

