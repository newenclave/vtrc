#ifndef VTRC_SYSTEM_FORWARD_H
#define VTRC_SYSTEM_FORWARD_H

#if defined(ASIO_STANDALONE)
#define VTRC_SYSTEM_FORWARD( x ) namespace asio { x }
#define VTRC_SYSTEM ::asio
#else
#define VTRC_SYSTEM_FORWARD( x ) namespace boost { namespace system { x } }
#define VTRC_SYSTEM ::boost::system
#endif

#endif // VTRC_SYSTEM_FORWARD_H

