/* omnius/ragasm.c
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 *
 *
 * These routines act on instances of type ragasm_t.
 *
 * All of the routines expect a caller-allocated ragasm_t object
 * as a last parameter. This is the ragasm the routine will operate
 * on. All of the routines return return EXIT_SUCCESS except,
 *      ragasm_validate,
 *      ragasm_clone_comment, and
 *      ragasm_clone,
 * which return EXIT_SUCCESS or EXIT_FAILURE depending upon their success.
 *
 *
 * 2015 - Mike Clark
 */

#include <string.h>
#include <stdlib.h>
#include "global.h"
#include "ragasm.h"




/* Bind a ragasm instance to FSM descriptor. This is the FSM the ragasm will hold state for and act as.
 * (TODO: word better)
 */
int
ragasm_load(fsm_descriptor_t *fsm_desc, ragasm_t *ragasm)
{
    fsm_desc->ref_count++;
    ragasm->fsm_desc = fsm_desc;
    /* the start state is always 1. Zero'th state is the invalid sink. */
    ragasm->curr_state = FSM_START_STATE;
    ragasm->prev_state = FSM_NULL_STATE;
    ragasm->is_loaded = TRUE;
    
    return EXIT_SUCCESS;
}

/* This will reset a FSM, unbind it from it's FSM descriptor. The caller is responsible for deallocating the ragasm
 * instance
 */
int
ragasm_unload(ragasm_t *ragasm)
{
    /* The FSM descriptor will not be able to unload successfully if it has outstanding references */
    ragasm->fsm_desc->ref_count--;
    ragasm->fsm_desc = NULL;

    /* the start state is always 1. Zero'th state is the invalid sink. */
    ragasm->curr_state = FSM_NULL_STATE;
    ragasm->prev_state = FSM_NULL_STATE;
    ragasm->is_loaded = FALSE;
    
    return EXIT_SUCCESS;
}



/* This sets the FSM to it's start state and resets the prev_state to NULL. */
int
ragasm_reload(ragasm_t *ragasm)
{
    ragasm->is_loaded = FALSE;
    ragasm->curr_state = FSM_START_STATE;
    ragasm->prev_state = FSM_NULL_STATE;
    ragasm->is_loaded = TRUE;
    
    return EXIT_SUCCESS;
}


/* FSM goto previous state */
int
ragasm_step_back(ragasm_t *ragasm)
{
    ragasm->curr_state = ragasm->prev_state;
    ragasm->prev_state = FSM_NULL_STATE;
    
    return EXIT_SUCCESS;
}

/* FSM goto next state on input symbol */
int
ragasm_step(SYMBOL_T symbol, ragasm_t *ragasm)
{
    ragasm->prev_state = ragasm->curr_state;
    ragasm->curr_state = *(ragasm->curr_state * ragasm->fsm_desc->symbol_count + symbol + ragasm->fsm_desc->jmp_tbl);
    
    return EXIT_SUCCESS;
}

/* FSM goto invalid sink without consuming an input.
 * Set prev_node to NULL making this irreversable (i.e. no step_back)
 */
int
ragasm_invalidate(ragasm_t *ragasm)
{
    ragasm->curr_state = FSM_NULL_STATE;
    ragasm->prev_state = FSM_NULL_STATE;
    
    return EXIT_SUCCESS;
}


/* Test if the FSM is valid (not in the NULL state)
 *  EXIT_SUCCESS : valid
 *  EXIT_FAILURE : invalid
 */
int
ragasm_validate(ragasm_t *ragasm)
{
    return ragasm->curr_state == FSM_NULL_STATE ? EXIT_FAILURE: EXIT_SUCCESS;
}




/* Copy a char sequence comment and append a cloned marker.
 * The src is expected to be NULL terminated.
 */
int
ragasm_clone_comment(char **dst_p, char *src) {
    int ret = EXIT_FAILURE;
    char *duplicate;
    size_t len;

    /* allocate one more space to append a clone marker */
    len = strlen(src);
    if (len > 0 && (duplicate = (char *) calloc((len + 1), sizeof(char)))) {
        memcpy(duplicate, src, len);
        duplicate[len - 1] = COMMENT_CLONE_MARK;
        duplicate[len] = '\0';
        *dst_p = duplicate;
        ret = EXIT_SUCCESS;
    }
    return ret;
}

/* Clone a ragasm - NOTE: this does not clone the data that ragasm is supervising, just allocates new memory with the
 * same policy, and it puts the FSM in the same state.
 */
int
ragasm_clone(ragasm_t *duplicate, ragasm_t *ragasm) {
    int ret = EXIT_FAILURE;

    duplicate->fsm_desc = ragasm->fsm_desc;
    duplicate->curr_state = ragasm->curr_state;
    duplicate->prev_state = ragasm->prev_state;
    if (ragasm->comment) {
        ret = ragasm_clone_comment(&duplicate->comment, ragasm->comment);
    } else {
        ret = EXIT_SUCCESS;
    }
    /*  Make sure _is_loaded is set within the locks */
    duplicate->is_loaded = ret == EXIT_SUCCESS ? TRUE : FALSE;
    return ret;
}
