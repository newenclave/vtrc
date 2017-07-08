
#include <map>

#include "vtrc-mutex.h"
#include "vtrc/common/connection-list.h"

namespace vtrc { namespace common {

    namespace {

        typedef common::connection_iface_sptr connection_sptr;
        typedef common::connection_iface_wptr connection_wptr;

        typedef std::map <
            common::connection_iface *, connection_sptr
        > client_map_type;
    }

    struct connection_list::impl {

        client_map_type     clients_;
        mutable vtrc::mutex clients_lock_;
        typedef vtrc::lock_guard<vtrc::mutex> locker_type;

        void clear( )
        {
            locker_type l(clients_lock_);
            clients_.clear( );
        }

        void store( common::connection_iface *c )
        {
            DEBUG_LINE(c);
            store( c->shared_from_this( ) );
        }

        void store( common::connection_iface_sptr c )
        {
            locker_type l(clients_lock_);
            clients_.insert(std::make_pair(c.get( ), c));
        }

        connection_sptr lock( common::connection_iface *c )
        {
            connection_sptr result;
            locker_type l(clients_lock_);
            client_map_type::iterator f( clients_.find( c ) );
            if( f != clients_.end( ) )
                result = f->second;
            return result;
        }

        void drop ( common::connection_iface *c )
        {
            locker_type lck(clients_lock_);
            client_map_type::iterator f(clients_.find(c));
            if( f != clients_.end( )) {
                clients_.erase( f );
            }
        }

        void drop ( common::connection_iface_sptr &c )
        {
            drop( c.get( ) );
        }

        size_t foreach_while(client_predic func)
        {
            locker_type l(clients_lock_);
            size_t result = 0;

            client_map_type::iterator b(clients_.begin( ));
            const client_map_type::iterator e(clients_.end( ));

            for( ; b!=e; ++b ) {
                bool ot = func( b->second );
                result += ot;
                if( !ot ) break;
            }
            return result;
        }
    };

    connection_list::connection_list( )
        :impl_(new impl)
    { }

    vtrc::shared_ptr<connection_list> connection_list::create( )
    {
        vtrc::shared_ptr<connection_list> new_inst(new connection_list);

        return new_inst;
    }

    connection_list::~connection_list( )
    {
        delete impl_;
    }

    void connection_list::clear( )
    {
        impl_->clear( );
    }

    void connection_list::store( common::connection_iface *connection )
    {
        impl_->store( connection );
    }

    void connection_list::store(common::connection_iface_sptr connection)
    {
        impl_->store( connection );
    }

    void connection_list::drop ( common::connection_iface *connection )
    {
        impl_->drop( connection );
    }

    void connection_list::drop(common::connection_iface_sptr connection)
    {
        impl_->drop( connection );
    }

    size_t connection_list::foreach_while(client_predic func)
    {
        return impl_->foreach_while( func );
    }

}}
