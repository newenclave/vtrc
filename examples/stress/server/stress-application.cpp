#include "stress-application.h"
#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-pool-pair.h"

namespace stress {

    using namespace vtrc;

    namespace {
        class app_impl: public server::application {
        public:
            app_impl( common::pool_pair &pp )
                :server::application(pp)
            { }
        };
    }

    struct application::impl {

        common::pool_pair   pp_;
        server::application app_;

        impl( unsigned io_threads )
            :pp_(io_threads)
            ,app_(pp_)
        { }

        impl( unsigned io_threads, unsigned rpc_threads )
            :pp_(io_threads, rpc_threads)
            ,app_(pp_)
        { }
    };

    application::application( unsigned io_threads )
        :impl_(new impl(io_threads))
    { }

    application::application( unsigned io_threads, unsigned rpc_threads )
        :impl_(new impl(io_threads, rpc_threads))
    { }

    server::application &application::get_application( )
    {
        return impl_->app_;
    }

    const server::application &application::get_application( ) const
    {
        return impl_->app_;
    }

}

