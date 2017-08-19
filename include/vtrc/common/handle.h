#ifndef VTRC_HANDLE_H
#define VTRC_HANDLE_H

#include "vtrc-stdint.h"

namespace vtrc { namespace common {
    struct native_handle_type {
        typedef void *pvoid;
        typedef pvoid handle;
        union value_type {
#ifdef _WIN32
            handle          win_handle;
#else
            int             unix_fd;
#endif
            handle          ptr_;
            vtrc::uintptr_t integer_;
        } value;
    };
}}

#endif // VTRC_HANDLE_H
