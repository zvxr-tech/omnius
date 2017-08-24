/* omnius/global.h
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 * Global header.
 *
 * 2015 - Mike Clark
 */
#ifndef SECMEM_GLOBAL_H
#define SECMEM_GLOBAL_H

#ifndef SECMEM_KERNEL
#include <stdint.h>
#endif /* SECMEM_KERNEL */

#ifdef SECMEM_INTERNAL_BIT
    #ifndef SECMEM_INTERNAL_T
        #if SECMEM_INTERNAL_BIT == 32
            typedef  uint32_t SECMEM_INTERNAL_T;
            #define MAX_SECMEM_INTERNAL 4294967295 /* 2^32 -1 */
        #elif SECMEM_INTERNAL_BIT == 64
            typedef  uint64_t SECMEM_INTERNAL_T;
            #define MAX_SECMEM_INTERNAL 18446744073709551615 /* 2^64 -1 */
        #else
            #error SECMEM_INTERNAL_BIT only supports '32' or '64'.
        #endif /* SECMEM_INTERNAL_BIT */
    #endif /* SECMEM_INTERNAL_T */
#else
    #define SECMEM_INTERNAL_BIT 64
    typedef  uint64_t SECMEM_INTERNAL_T;
    #define MAX_SECMEM_INTERNAL 18446744073709551615 /* 2^64 -1 */
#endif /* SECMEM_INTERNAL_BIT */

#define OMNIUS_DEFAULT_MSG_IN 0xdead1
#define OMNIUS_DEFAULT_MSG_OUT 0xdead2

#ifndef FALSE
    #define FALSE 0
#endif /* FALSE */
#ifndef TRUE
    #define TRUE 1
#endif /* TRUE */
#endif /* SECMEM_GLOBAL_H */
