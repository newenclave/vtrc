#ifndef VTRC_CXX11_H
#define VTRC_CXX11_H
#include "vtrc-general-config.h"

#if VTRC_DISABLE_CXX11
#define VTRC_NULL       NULL
#define VTRC_NOEXCEPT

#define VTRC_DISABLE_COPY(ClassName) \
    private: \
        ClassName( const ClassName & );\
        ClassName& operator = ( const ClassName & )

#else

#define VTRC_NULL           nullptr
#define VTRC_NOEXCEPT       noexcept
#define VTRC_DISABLE_COPY(ClassName) \
    public: \
        ClassName( const ClassName & ) = delete; \
        ClassName& operator = ( const ClassName & ) = delete
#endif


#endif // VTRCCXX11_H
