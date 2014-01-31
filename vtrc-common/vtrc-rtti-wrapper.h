#ifndef VTRC_RTTI_WRAPPER_H
#define VTRC_RTTI_WRAPPER_H

#include <typeinfo>

namespace vtrc { namespace common {

    class type_rtti_wrapper
    {
        const std::type_info* info_;

    public:

        type_rtti_wrapper()
        {
            struct null_info {};
            info_ = &typeid(null_info);
        }

        explicit type_rtti_wrapper(const std::type_info &info)
          :info_(&info)
        {}

        const std::type_info& get() const
        {
            return *info_;
        }

        bool before(const type_rtti_wrapper& rhs) const
        {
            return !!info_->before(*rhs.info_);
        }

        const char* name() const
        {
            return info_->name( );
        }
    };

    inline bool operator < (const type_rtti_wrapper& left,
                            const type_rtti_wrapper& right)
    { return left.before(right); }

}}

#endif
