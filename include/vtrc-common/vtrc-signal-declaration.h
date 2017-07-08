#ifndef VTRC_SIGNAL_DECLARATION_H
#define VTRC_SIGNAL_DECLARATION_H

#include "observer/simple.h"

#include "vtrc-mutex.h"

//#define VTRC_PP_CONCAT( a, b ) a ## b
#include "observer/simple.h"

#define VTRC_DECLARE_SIGNAL_COMMON( Access, Name, SigType, MutexType ) \
    public:  \
        typedef vtrc::common::observer::simple< \
            SigType, MutexType \
        > Name##_type;  \
        typedef typename Name##_type::slot_type Name##_slot_type; \
        typename Name##_type::connection Name##_connect( Name##_slot_type s ) \
        { \
            return Name##_.connect(s); \
        } \
    Access: \
        Name##_type&  get_##Name( ) \
        {  \
            return Name##_; \
        }  \
    private: \
        Name##_type Name##_

// ====== SAFE

#define VTRC_DECLARE_SIGNAL_SAFE(   Name, SigType )             \
        VTRC_DECLARE_SIGNAL_COMMON( protected, Name, SigType,   \
                                    vtrc::mutex )

// ====== UNSAFE
#define VTRC_DECLARE_SIGNAL_UNSAFE( Name, SigType )             \
        VTRC_DECLARE_SIGNAL_COMMON( protected, Name, SigType,   \
                                     vtrc::dummy_mutex)

#define VTRC_DECLARE_SIGNAL VTRC_DECLARE_SIGNAL_SAFE

#endif // VTRC_SIGNAL_DECLARATION_H
