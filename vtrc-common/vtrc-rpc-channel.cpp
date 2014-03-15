
#include <map>
#include <google/protobuf/descriptor.h>

#include "vtrc-rpc-channel.h"
#include "vtrc-memory.h"
#include "vtrc-mutex.h"
#include "vtrc-mutex-typedefs.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"
#include "proto-helper/message-utilities.h"

namespace vtrc { namespace common  {

    namespace {

        typedef std::map <
             const google::protobuf::MethodDescriptor *
            ,vtrc::shared_ptr<vtrc_rpc_lowlevel::options>
        > options_map_type;
    }

    struct rpc_channel::impl {

        options_map_type           options_map_;
        mutable vtrc::shared_mutex options_map_lock_;

        const vtrc_rpc_lowlevel::options &select_options(
                        const google::protobuf::MethodDescriptor *method)
        {
            upgradable_lock lck(options_map_lock_);

            options_map_type::const_iterator f(options_map_.find(method));

            vtrc::shared_ptr<vtrc_rpc_lowlevel::options> result;

            const vtrc_rpc_lowlevel::service_options_type &serv (
               method->service( )->options( )
                    .GetExtension( vtrc_rpc_lowlevel::service_options ));

            const vtrc_rpc_lowlevel::method_options_type &meth (
               method->options( )
                  .GetExtension( vtrc_rpc_lowlevel::method_options));

            if( f == options_map_.end( ) ) {
                result = vtrc::make_shared<vtrc_rpc_lowlevel::options>
                                                                (serv.opt( ));
                if( meth.has_opt( ) )
                    utilities::merge_messages( *result, meth.opt( ) );

                std::cout << "\nopt: " << result->DebugString( ) << "\n";

                upgrade_to_unique ulck( lck );
                options_map_.insert( std::make_pair( method, result ) );

            } else {
                result = f->second;
            }

            return *result;
        }
    };

    rpc_channel::rpc_channel( )
        :impl_(new impl)
    {}

    rpc_channel::~rpc_channel( )
    {
        delete impl_;
    }

    const vtrc_rpc_lowlevel::options &rpc_channel::select_options(
                            const google::protobuf::MethodDescriptor *method)
    {
        return impl_->select_options( method );
    }

}}

