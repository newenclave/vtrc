
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread.hpp>
#include <boost/weak_ptr.hpp>
#include <map>

#include "vtrc-connection-list.h"
#include "vtrc-common/vtrc-connection-iface.h"

namespace vtrc { namespace server {

    namespace {

        typedef boost::shared_ptr<common::connection_iface> connection_sptr;
        typedef boost::weak_ptr  <common::connection_iface> connection_wptr;

        typedef std::map <
            common::connection_iface *, connection_sptr
        > client_map_type;

        typedef boost::unique_lock<boost::shared_mutex>    unique_lock;
        typedef boost::shared_lock<boost::shared_mutex>    shared_lock;
    }

    struct connection_list::impl {

        client_map_type             clients_;
        mutable boost::shared_mutex clients_lock_;

        void store( common::connection_iface *c )
        {
            unique_lock l(clients_lock_);
            clients_.insert(
                    std::make_pair(c,
                            boost::shared_ptr<common::connection_iface>(c)));
        }

        connection_sptr lock( common::connection_iface *c )
        {
            connection_sptr result;
            shared_lock l(clients_lock_);
            client_map_type::iterator f( clients_.find( c ) );
            if( f != clients_.end( ) )
                result = f->second;
            return result;
        }

        void drop ( common::connection_iface *c )
        {
            unique_lock l(clients_lock_);
            client_map_type::iterator f(clients_.find(c));
            if( f != clients_.end( )) clients_.erase( f );
        }

        size_t foreach_while(client_predic func)
        {
            shared_lock l(clients_lock_);
            size_t result = 0;

            for(client_map_type::iterator b(clients_.begin()),
                                          e(clients_.end()); b!=e; ++b )
            {
                bool ot = func( b->first );
                result += ot;
                if( !ot ) break;
            }
            return result;
        }
    };

    connection_list::connection_list()
        :impl_(new impl)
    { }

    connection_list::~connection_list()
    {
        delete impl_;
    }

    void connection_list::store( common::connection_iface *connection )
    {
        impl_->store( connection );
    }

    void connection_list::drop ( common::connection_iface *connection )
    {
        impl_->drop( connection );
    }

    connection_sptr connection_list::lock(common::connection_iface *connection)
    {

    }

    size_t connection_list::foreach_while(client_predic func)
    {
        return impl_->foreach_while( func );
    }

}}
