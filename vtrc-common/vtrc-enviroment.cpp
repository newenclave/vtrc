
#include "vtrc-enviroment.h"

#include <map>
#include "vtrc-mutex-typedefs.h"

namespace vtrc { namespace common {

    struct enviroment::impl {

        typedef std::map<std::string, std::string> data_type;

        data_type             data_;
        mutable shared_mutex  data_lock_;

        impl( ) { }

        impl( const impl &other_data )
        {
            other_data.get_data(data_);
        }

        void get_data( data_type &out ) const
        {
            data_type tmp;
            shared_lock l(data_lock_);
            tmp.insert(data_.begin( ), data_.end( ));
            out.swap( tmp );
        }

        void copy_from( const impl &other_data )
        {
            data_type tmp;
            other_data.get_data( tmp );

            unique_shared_lock l(data_lock_);
            data_.swap( tmp );
        }

        void set( const std::string &name, const std::string &value )
        {
            unique_shared_lock l(data_lock_);

            data_type::iterator f(data_.find(name));

            if( f == data_.end( ) ) {
                data_.insert( std::make_pair(name, value) );
            } else {
                f->second.assign(value);
            }
        }

        const std::string get( const std::string &name ) const
        {
            const static std::string empty_;
            return get(name, empty_);
        }

        const std::string get( const std::string &name,
                                const std::string &default_value ) const
        {
            shared_lock l(data_lock_);
            data_type::const_iterator f(data_.find(name));
            if( f == data_.end( ) ) {
                return default_value;
            } else {
                return f->second;
            }
        }

    };

    enviroment::enviroment( )
        :impl_(new impl)
    {}

    enviroment::enviroment( const enviroment &other )
        :impl_(new impl(*other.impl_))
    {}

    enviroment &enviroment::operator = ( const enviroment &other )
    {
        impl_->copy_from( *other.impl_ );
        return *this;
    }

    enviroment::~enviroment( )
    {
        delete impl_;
    }

    void enviroment::set( const std::string &name, const std::string &value )
    {
        impl_->set( name, value );
    }

    const std::string enviroment::get( const std::string &name ) const
    {
        return impl_->get( name );
    }

    const std::string enviroment::get( const std::string &name,
                                        const std::string &default_value ) const
    {
        return impl_->get( name, default_value );
    }

}}

