/* omnius/ragasm.h
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 * 2015 - Mike Clark
 */

#ifndef SECMEM_RAGASM_H
#define SECMEM_RAGASM_H

#include <pthread.h>
#include "fsm_descriptor.h"

#define COMMENT_CLONE_MARK '@'
/*
A ragasm instance manages a FSM constructed from a FSM descriptor.
This is really the FSM if you think about it.
*/
typedef struct ragasm_t
{
    fsm_descriptor_t    *fsm_desc;  /*  Finite State Machine description */
    STATE_T             prev_state; /*  previous state index in the fsm_desc */
    STATE_T             curr_state; /*  current state index in the fsm_desc */
    char                is_loaded;  /*  is a fsm descriptor loaded */
    /* pointer to null-terminated char sequence. Can be used to put the plicy regex for reference */
    char                *comment;
} ragasm_t;

int
ragasm_load(fsm_descriptor_t *, ragasm_t *);

int
ragasm_unload(ragasm_t *);

int
ragasm_reload(ragasm_t *);

int
ragasm_step_back(ragasm_t *);

int
ragasm_step(SYMBOL_T, ragasm_t *);

int
ragasm_invalidate(ragasm_t *);

int
ragasm_validate(ragasm_t *);

int
ragasm_clone_comment(char **, char *);

int
ragasm_clone(ragasm_t *, ragasm_t *);

#endif /* SECMEM_RAGASM_H */
