/* omnius/memory.h
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 *
 *
 * 2015 - Mike Clark
 */
#include "global.h"
#include "ragasm.h"

#ifndef SECMEM_MEMORY_H
#define SECMEM_MEMORY_H

/*
 * Each process using omnius' services is provided a fixed-sized amount of memory (set when the process loads into
 * omnius). Each process can then be thought to have it's own secmem virtual memory region. The secmem address starts at
 * zero through to the size of the region. The secmem address is then mapped to a vm address within omnius's PAS by
 * basing it at the address of the start of the total memory allocation for the process in omnius's vm.
 *
 * For example (stripping other details for clarity),
 *  i)   omnius receives a load request from a process and the process wants a total size of 0x100 Bytes
 *  ii)  omnius allocates 0x100 bytes normally via calloc.
 *  iii) omnius saves the address of the allocation in the process object's base field. This value must never change for
 *       the life of that process object instance.
 *  iv)  Now the process has a secmem region of size 0x100. We virtualize this with respect to the processs.
 *  v)   allocations happen for that process, populating the memory and segmenting the original giant node into smaller
 *       nodes.
 *  Now to calculate the actual address that omnius has to access ini it's own PAS for a give secmem address of a
 *  process:
 *   actual vm address = proc.base + secmem_vm addr
 *
 *
 * The object has:
 *  an offset field (secmem vm address),
 *  size (in bytes) of the region,
 *  pointers (prev/next) to traverse the memory nodes,
 *  a flag to indicate if the memory is in use (allocated) with respect to the secmem vm,
 *  a ragasm object which manages the FSM which expresses the policy applied to this memory object.
 *
 */
typedef struct secmem_obj_t
{
    SECMEM_INTERNAL_T offset;
    SECMEM_INTERNAL_T size;
    struct secmem_obj_t *prev;
    struct secmem_obj_t *next;
    char used;
    ragasm_t ragasm;
} secmem_obj_t;


int
memory_get_obj_by_addr(SECMEM_INTERNAL_T, secmem_obj_t  **, secmem_obj_t *);

int
memory_load(SECMEM_INTERNAL_T, secmem_obj_t **);

int
memory_unload(secmem_obj_t *);

int
memory_alloc(SECMEM_INTERNAL_T, fsm_descriptor_t *, secmem_obj_t **, secmem_obj_t **);

int
memory_dealloc(secmem_obj_t *);

int
memory_read(SECMEM_INTERNAL_T, char *, secmem_obj_t *);

int
memory_write(SECMEM_INTERNAL_T, secmem_obj_t *);

#endif /* SECMEM_MEMORY_H */
