
#include <boost/asio.hpp>

#include "vtrc-transport-tcp.h"

namespace vtrc { namespace common {

    namespace bip = boost::asio::ip;

    struct transport_tcp::transport_tcp_impl {

        boost::shared_ptr<bip::tcp::socket> sock_;

        transport_tcp_impl( bip::tcp::socket *s )
            :sock_(s)
        {}

    };

    transport_tcp::transport_tcp( bip::tcp::socket *s )
        :impl_(new transport_tcp_impl(s))
    {}

}}
