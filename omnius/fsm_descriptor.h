/* omnius/fsm_descriptor.h
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 *
 * 2015 - Mike Clark
 */

#ifndef SECMEM_FSM_DESCRIPTOR_H
#define SECMEM_FSM_DESCRIPTOR_H


#include "global.h"


/*
 * These are used internally by fsm_descriptor routines. These routines are also expected to ensure that input
 * does not exceed the value that can be contained in each. The non-fsm_descriptor_t input parameters should only be of
 * types known to the layers above (SECMEM_INTERNAL_T), in this case (comm layer).
 *
 * TODO: There's got to be a better way to calculate the MAX's
 */
#define SYMBOL_T unsigned char
#define MAX_SYMBOL 255 /* 2^8 - 1 */
#define STATE_T  unsigned char
#define MAX_STATE 255 /* 2^8 - 1 */
#define MAX_FSM_COMMENT_LEN 32 /* keep less than maxof(SECMEM_INTERNAL_T) */

/* These are pre-defined states for FSM used by omnius. There are an innumerable number of dependencies on these two
 * constants, so think very carefully before modifying them
 */
#define FSM_NULL_STATE  0
#define FSM_START_STATE 1


/*
 * The fsm_descriptor_t type is used to describe a finite state machine, however, it has been purposely kept separate
 * from the state of a running machine. This allows for a one-to-one mapping between policies and fsm descriptors. Meaning
 * that for each policy specified for a process only one fsm_descriptor is instantiated, and for each secure memory allocation
 * in that process, a ragasm object will be generated to manage state, with reference to the fsm_descriptor for the
 * policy applied to the memory object resulting from the allocation.
 *
 */
typedef struct fsm_descriptor_t
{
    int ref_count; /* TODO use macro for something that acts atomically */
    SYMBOL_T symbol_count; /*  number of symbols in the alphabet (including NULL=0) */
    char *comment; /* null-terminated */
    SYMBOL_T *alpha_map; /* Mapping from external input char -> internal FSM input symbol [0,|symbols|] */
    STATE_T *jmp_tbl; /* pointer to the state jmp table that describes this FSM */
} fsm_descriptor_t;



int fsm_descriptor_load(char *, size_t, fsm_descriptor_t *);
int fsm_descriptor_unload(fsm_descriptor_t *);

#endif /* SECMEM_FSM_DESCRIPTOR_H */
