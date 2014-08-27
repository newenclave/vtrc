#ifndef VTRC_STDINT_H
#define VTRC_STDINT_H

//#include "config.h"

#ifdef _WIN32

#if _MSC_VER >= 1600

// Visual Studio 2010+
#include <stdint.h>

#else

typedef signed      char  int8_t;
typedef unsigned    char uint8_t;
typedef signed     short  int16_t;
typedef unsigned   short uint16_t;
typedef signed   __int32  int32_t;
typedef unsigned __int32 uint32_t;
typedef signed   __int64  int64_t;
typedef unsigned __int64 uint64_t;

#endif

#else

#include <stdint.h>

#endif

#endif // VTRC_STDINT_H
