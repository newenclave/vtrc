#ifndef VTRC_CXX11_H
#define VTRC_CXX11_H
#include "vtrc-general-config.h"

#if VTRC_DISABLE_CXX11
#define VTRC_NULL       NULL
#define VTRC_NOEXCEPT
#else
#define VTRC_NULL       nullptr
#define VTRC_NOEXCEPT   noexcept
#endif


#endif // VTRCCXX11_H
