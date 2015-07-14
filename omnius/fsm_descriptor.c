/* omnius/fsm_descriptor.c
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 *
 *
 * These routines expect the caller to allocate and deallocate the input fsm_descriptor reference parameter.
 * The load routine allocates the space for the comment field of the fsm_descriptor, and a subroutine it calls also
 * allocates space for the alpha_map and jmp_tbl.
 * The unload routine free's all the memory allocated by the load routine (and it's children).
 *
 * All of these routines return EXIT_SUCCESS or EXIT_FAILURE.
 *
 * 2015 - Mike Clark
 */
#include <stdlib.h>
#include <string.h>
#include "fsm_descriptor.h"
#include "regex_parse/regex_parse.h"



/*
 * Constuct an FSM descriptor from a regular expression char sequence. E={A-Za-z, '(', ')', '*', '?'} (ex. "R(WR)*W?RW"
 * The REGEX is NOT null-terminated, it's length is passed in as LEN.
 */
int
fsm_descriptor_load(char *regex, size_t len, fsm_descriptor_t *fsm_desc)
{
    SYMBOL_T *alpha_map;
    int ret = EXIT_FAILURE;;
    fsm_desc->ref_count = 0;

    /* The buffer len is for internal use, so we don't need to constrain it to SECMEM_INTERNAL_T,
     * however we still check to ensure that we are not overflowing it.
     */
    size_t buffer_len = len + 1;
    if (buffer_len > len) {
        char *buffer = (char *) calloc(buffer_len, sizeof(char));
        if (buffer) {
            memmove(buffer, regex, len);
            /* in case the above did not write the entire string if it was > comment_len. Null terminate. */
            buffer[buffer_len] = '\0';
            SYMBOL_T symbol_count;
            /* This routine will populate the symbol_count, alpha_map, and jmp_tbl */
            ret = compile_regex(buffer, &symbol_count, &fsm_desc->alpha_map, &fsm_desc->jmp_tbl);
            fsm_desc->symbol_count = symbol_count;
            fsm_desc->comment = buffer;
        }
    }
    return ret;
}

/* Unload and free memory of a fsm_descriptor. This will fail if ragasm objects have outstanding references to it */
int
fsm_descriptor_unload(fsm_descriptor_t *fsm_desc)
{
    int ret = EXIT_FAILURE;

    /*  ref_count should be zero at this point */
    if (fsm_desc->ref_count == 0)
    {
        if (fsm_desc->comment)
            free(fsm_desc->comment);
        if (fsm_desc->jmp_tbl)
            free(fsm_desc->jmp_tbl);
        if (fsm_desc->alpha_map)
            free(fsm_desc->alpha_map);
        ret = EXIT_SUCCESS;
    }
    return ret;
}
