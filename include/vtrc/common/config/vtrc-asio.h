#ifndef VTRC_ASIO_H
#define VTRC_ASIO_H


#include "vtrc-asio-forward.h"

#if defined(ASIO_STANDALONE)
#   include "asio.hpp"
#else
#   include "vtrc-asio-forward.h"
#   include "boost/asio.hpp"
#endif

#endif // VTRC_ASIO_H

