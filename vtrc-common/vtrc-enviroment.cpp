
#include "vtrc-enviroment.h"

#include <boost/thread/shared_mutex.hpp>
#include <map>

namespace vtrc { namespace common {

    namespace {
        typedef boost::unique_lock<boost::shared_mutex> unique_lock;
        typedef boost::shared_lock<boost::shared_mutex> shared_lock;
    }

    struct enviroment::enviroment_impl {

        typedef std::map<std::string, std::string> data_type;

        data_type                   data_;
        mutable boost::shared_mutex data_lock_;

        enviroment_impl( ) {}
        enviroment_impl( const data_type &other_data )
            :data_(other_data)
        {}

        void set( const std::string &name, const std::string &value )
        {
            unique_lock l(data_lock_);
            data_type::iterator f(data_.find(name));
            if( f == data_.end( ) ) {
                data_.insert( std::make_pair(name, value) );
            } else {
                f->second.assign(value);
            }
        }

        const std::string &get( const std::string &name ) const
        {
            const static std::string empty_;
            shared_lock l(data_lock_);
            data_type::const_iterator f(data_.find(name));
            if( f == data_.end( ) ) {
                return empty_;
            } else {
                return f->second;
            }
        }

    };

    enviroment::enviroment( )
        :impl_(new enviroment_impl)
    {}

    enviroment::enviroment( const enviroment &other )
        :impl_(new enviroment_impl(other.impl_->data_))
    {}

    enviroment::~enviroment( )
    {
        delete impl_;
    }

    void enviroment::set( const std::string &name, const std::string &value )
    {
        impl_->set( name, value );
    }

    const std::string &enviroment::get( const std::string &name ) const
    {
        return impl_->get( name );
    }

}}

