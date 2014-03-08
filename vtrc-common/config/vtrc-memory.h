#ifndef VTRC_MEMORY_H
#define VTRC_MEMORY_H

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace vtrc {

    using boost::shared_ptr;
    using boost::scoped_ptr;
    using boost::weak_ptr;
    using boost::make_shared;
    using boost::enable_shared_from_this;

//    template <typename T>
//    struct shared_ptr { typedef boost::shared_ptr<T> type; };

//    template <typename T>
//    struct weak_ptr { typedef boost::weak_ptr<T> type; };
}

#endif // VTRCMEMORY_H
