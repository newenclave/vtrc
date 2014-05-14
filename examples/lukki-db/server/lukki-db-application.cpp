
#include "lukki-db-application.h"
#include "vtrc-common/vtrc-thread-pool.h"

namespace lukki_db {

    using namespace vtrc;

    struct  application::impl {

        common::pool_pair   &pool_;
        common::thread_pool db_thread_;

        impl( common::pool_pair &p )
            :pool_(p)
            ,db_thread_(1)
        { }
    };

    application::application( vtrc::common::pool_pair &pp )
        :impl_(new impl(pp))
    {

    }

    application::~application( )
    {
        delete impl_;
    }
}

