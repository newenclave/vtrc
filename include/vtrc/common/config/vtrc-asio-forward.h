#ifndef VTRC_ASIO_FORWARD_H
#define VTRC_ASIO_FORWARD_H

#if defined(ASIO_STANDALONE)
#define VTRC_ASIO_FORWARD( x ) namespace asio { x }
#define VTRC_ASIO ::asio
#else
#define VTRC_ASIO_FORWARD( x ) namespace boost { namespace asio { x } }
#define VTRC_ASIO ::boost::asio
#endif


#endif // VTRC_VTRC_ASIO_FORWARD_H

