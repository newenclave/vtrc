#ifndef VTRC_COMMON_OBSERVERS_SIMPLE_H
#define VTRC_COMMON_OBSERVERS_SIMPLE_H

#include "traits/simple.h"
#include "base.h"

namespace vtrc { namespace common { namespace observer {

#if !VTRC_DISABLE_CXX11
    template <typename T, typename MutexType = vtrc::mutex>
    using simple = base<traits::simple<T>, MutexType>;
#else
    template <typename T, typename MutexType = vtrc::mutex>
    class simple: public base<traits::simple<T>, MutexType> {
        typedef base<traits::simple<T>, MutexType> parent_type;
    public:
        typedef typename parent_type::slot_type  slot_type;
    };
#endif

}}}
#endif // SIMPLE_H
