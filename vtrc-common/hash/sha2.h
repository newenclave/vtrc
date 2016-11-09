#include "vtrc-general-config.h"

#if VTRC_OPENSSL_ENABLED
#include "openssl/sha2.h"
#else
#   include "sha2.himpl"
#endif
