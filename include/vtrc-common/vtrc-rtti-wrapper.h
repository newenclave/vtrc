#ifndef VTRC_RTTI_WRAPPER_H
#define VTRC_RTTI_WRAPPER_H

#include <typeinfo>

namespace vtrc { namespace common {

    class rtti_wrapper
    {
        const std::type_info* info_;
        struct null_info { };

    public:

        rtti_wrapper( )
            :info_(&typeid(null_info))
        {}

        explicit rtti_wrapper(const std::type_info &info)
            :info_(&info)
        {}

        const std::type_info& get( ) const
        {
            return *info_;
        }

        bool before(const rtti_wrapper& rhs) const
        {
            return !!info_->before(*rhs.info_);
        }

        const char* name() const
        {
            return info_->name( );
        }
    };

    inline bool operator < (const rtti_wrapper& left,
                            const rtti_wrapper& right)
    { return left.before(right); }

}}

#endif
