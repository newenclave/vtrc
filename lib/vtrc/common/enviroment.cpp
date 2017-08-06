
#include "vtrc/common/environment.h"

#include <map>
#include "vtrc-mutex.h"

namespace vtrc { namespace common {

    struct environment::impl {

        typedef std::map<std::string, std::string> data_type;
        typedef vtrc::lock_guard<vtrc::mutex> locker_type;

        data_type             data_;
        mutable vtrc::mutex   data_lock_;

        impl( ) { }

        impl( const impl &other_data )
        {
            other_data.get_data(data_);
        }

        void get_data( data_type &out ) const
        {
            locker_type l(data_lock_);
            data_type tmp(data_);
            out.swap( tmp );
        }

        void copy_from( const impl &other_data )
        {
            data_type tmp;
            other_data.get_data( tmp );

            locker_type l(data_lock_);
            data_.swap( tmp );
        }

        void set( const std::string &name, const std::string &value )
        {
            locker_type l(data_lock_);

            data_type::iterator f(data_.find(name));

            if( f == data_.end( ) ) {
                data_.insert( std::make_pair(name, value) );
            } else {
                f->second.assign(value);
            }
        }

        const std::string get( const std::string &name ) const
        {
            static const std::string empty_;
            return get(name, empty_);
        }

        const std::string get( const std::string &name,
                               const std::string &default_value ) const
        {
            locker_type l(data_lock_);
            data_type::const_iterator f(data_.find(name));
            if( f == data_.end( ) ) {
                return default_value;
            } else {
                return f->second;
            }
        }

        size_t remove( const std::string &name )
        {
            locker_type l(data_lock_);
            return data_.erase( name );
        }

        bool exists( const std::string &name ) const
        {
            locker_type l(data_lock_);
            data_type::const_iterator f(data_.find(name));
            return f != data_.end( );
        }

    };

    environment::environment( )
        :impl_(new impl)
    { }

    environment::environment( const environment &other )
        :impl_(new impl(*other.impl_))
    { }

    environment &environment::operator = ( const environment &other )
    {
        impl_->copy_from( *other.impl_ );
        return *this;
    }

    environment::~environment( )
    {
        delete impl_;
    }

    void environment::set( const std::string &name, const std::string &value )
    {
        impl_->set( name, value );
    }

    const std::string environment::get( const std::string &name ) const
    {
        return impl_->get( name );
    }

    const std::string environment::get( const std::string &name,
                                       const std::string &default_value ) const
    {
        return impl_->get( name, default_value );
    }

    size_t environment::remove( const std::string &name )
    {
        return impl_->remove( name );
    }

    bool environment::exists( const std::string &name ) const
    {
        return impl_->exists( name );
    }

}}

