/* this tries to emulate a stdint.h */

#ifndef __PSSTDINT_H__
#define __PSSTDINT_H__

#include "cstypes.h"

// Remove *_t definitions to be consistent with CS types

typedef int8	int8_t;
typedef uint8	uint8_t;
typedef int16	int16_t;
typedef uint16	uint16_t;
typedef int32	int32_t;
typedef uint32	uint32_t;
typedef int64  int64_t;
typedef uint64 uint64_t;

#endif
