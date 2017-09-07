#ifndef VTRC_COMMON_OBSERVERS_DEFINE_H
#define VTRC_COMMON_OBSERVERS_DEFINE_H

#include "vtrc/common/observer/base.h"
#include "vtrc/common/observer/simple.h"
#include "vtrc-mutex.h"

#define VTRC_OBSERVER_DEFINE_COMMON( Name, Sig, Visible, Mutex )           \
    public:                                                                \
        typedef vtrc::common::observer::simple<Sig, Mutex> Name##_type;    \
        typedef typename Name##_type::slot_type                            \
                         Name##_slot_type;                                 \
        vtrc::common::observer::subscription                               \
        subscribe_##Name( Name##_slot_type slot )                          \
        {                                                                  \
            return Name.subscribe( slot );                                 \
        }                                                                  \
    Visible:                                                               \
        Name##_type Name

#define VTRC_OBSERVER_DEFINE_SAFE( Name, Sig ) \
      VTRC_OBSERVER_DEFINE_COMMON( Name, Sig, protected, vtrc::recursive_mutex )

#define VTRC_OBSERVER_DEFINE_UNSAFE( Name, Sig ) \
        VTRC_OBSERVER_DEFINE_COMMON( Name, Sig, protected, vtrc::dummy_mutex )

#define VTRC_OBSERVER_DEFINE VTRC_OBSERVER_DEFINE_SAFE

#endif // COMMON_OBSERVERS_DEFINE_H
