/* omnius/process.h
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 *
 * 2015 - Mike Clark
 */

#ifndef SECMEM_PROCESS_H
#define SECMEM_PROCESS_H

#include <signal.h>
#include "global.h"
#include "memory.h"
#include "fsm_descriptor.h"
#include "comm.h"


/*
 * This is used to describe a process that has been registered with omnius (via LOAD message).
 * One instance per process (measured by pid).
 */
typedef struct secmem_process_t
{
    pid_t pid;
    /* VM address (with respect to the omnius program) that maps to the beginning of the inferior processes secure
     * memory (secmem) region. This is used to access the actual memory backing the secmem vm.
     */
    char *base;
    /* Total amount of secmem memory for the process. This is fixed when the process registers with omnius via load */
    SECMEM_INTERNAL_T mem_size;
    /* Pointer to the first node in a list of memory object nodes that together form the entire secmem area */
    secmem_obj_t *secmem_head;
    /* number of policies loaded for this process */
    SECMEM_INTERNAL_T fsm_count;
    /* pointer to an array of fsm_descriptors. One for each policy the process has access to. The ordering of the aray
     * implies the policy id of the element by it's index.
     */
    fsm_descriptor_t *fsm_desc;
} secmem_process_t;

int process_load     (blob_t *, secmem_process_t *);
int process_unload   (blob_t *, secmem_process_t *);
int process_alloc    (blob_t *, secmem_process_t *);
int process_dealloc  (blob_t *, secmem_process_t *);
int process_read     (blob_t *, secmem_process_t *);
int process_write    (blob_t *, secmem_process_t *);


#endif /* SECMEM_PROCESS_H */
