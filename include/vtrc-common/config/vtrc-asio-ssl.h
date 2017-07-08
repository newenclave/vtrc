#ifndef VTRC_ASIO_SSL_H
#define VTRC_ASIO_SSL_H

#include "vtrc-asio-forward.h"

#if ASIO_STANDALONE
#   include "asio/ssl.hpp"
#else
#   include "boost/asio/ssl.hpp"
#endif

#endif // VTRC_ASIOSSL_H

