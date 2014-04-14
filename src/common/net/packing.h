// Hacks for different compiler to pack structs
#ifndef __PACKING_H__
#define __PACKING_H__

/**
 * \addtogroup common_net
 * @{ */

#ifdef __GNUC__
#   define PS_PACKED   __attribute__((packed))
#else
#   define PS_PACKED
#endif

/** @} */

#endif

