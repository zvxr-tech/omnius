/* omnius/memory.c
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 * The memory layer operates on the data structures associated with memory objects (secmem_obj_).
 *
 * The load and alloc routine expect a pointer to a memory object pointer. They use this to place the address from an
 * allocatiom of a secmem_obj.The rest of the routines expect a memory obect pointer,
 *
 * All of these routines return EXIT_SUCCESS or EXIT_FAILURE.
 *
 * 2015 - Mike Clark
 */

#include <stdlib.h>
#include <string.h>
#include "memory.h"

/* This will allocate one new memory object and point the head to it,
 * The size should be the entire memory size requested by the process.
 */
int
memory_load(SECMEM_INTERNAL_T size, secmem_obj_t **head)
{
    int ret = EXIT_FAILURE;
    if ((*head = (secmem_obj_t *) calloc(1, sizeof(secmem_obj_t))) != NULL) {
        (*head)->size = size;
        ret = EXIT_SUCCESS;
    }
    return ret;
}


/* Unload all memory objects starting from a HEAD memory object. This routine is used when unload  a process and
 * the all memory objects must be destroyed.
 */
int
memory_unload(secmem_obj_t *head)
{
    secmem_obj_t *h;
    int ret = EXIT_SUCCESS;

    while (head) {
        if (head->used)
            ret |= ragasm_unload(&head->ragasm); /* accumulate errors - only works cause failure != 0 */
        h = head->next;
        free(head);
        head = h;
    }

    return ret;
}

/*
 * This routine is used to allocate a new memory object of a given size for a given process using a first-fit method.
 *
 * PARAMETERS
 * size - size of allocation in bytes
 *
 * dsm_desc - the fsm description whose policy will apply to the allocation. A ragel will be generated to realize
 *              FSM instance by maintaining state and computing transitions on input symbols fed to it
 *              (i.e. 'R' read, and 'W' write)
 *
 * node_p - location to place a pointer to the newly allocated memory node. This is used to pass back a reference to the
 *          node because it is needed for further processing by the calling function in the layer above (process).
 *
 * head_p - a pointer to a pointer to the first node in the list of memory objects to allocate from. We use the
 *          double-indirection so that we can modify the head pointer in case we allocate a new memory object as the
 *          head of the list and need ti change the head pointer.
 *
 *  RETURN
 *  EXIT_SUCCESS on success, otherwise failure.
 *
 *  NOTES
 *
 */
int
memory_alloc(SECMEM_INTERNAL_T size, fsm_descriptor_t *fsm_desc, secmem_obj_t **node_p, secmem_obj_t **head_p) {
    int ret = EXIT_FAILURE;
    secmem_obj_t *new_node = NULL;
    secmem_obj_t *head = *head_p;
    while (head) {
        if (!head->used && size <= head->size) {
            if (head->size != size) {
                new_node = (secmem_obj_t *) calloc(1, sizeof(secmem_obj_t));
                if (new_node) {
                    /* Fix new node */
                    new_node->offset = head->offset;
                    new_node->size = size;
                    new_node->prev = head->prev;
                    new_node->next = head;
                    /*new_node->ragasm is allocated and setup by the caller in the layer above */

                    /* Fix old node (ahead now)*/
                    head->offset += size;
                    head->size -= size;
                    head->prev = new_node;

                    /* Fix node behind and test to see if the allocation was the first node, if so, point the memory
                     * node list head to it. ASSUMES that the only node with a NULL prev pointer is the head
                     */
                    if (new_node->prev) {
                        new_node->prev->next = new_node;
                    } else {
                        *head_p = new_node;
                    }
                    new_node->used = TRUE;
                    ret = EXIT_SUCCESS;
                } /* else ret = EXIT_FAILURE */
            } else {
                /* if candidate size is equal to the memory node we are looking at,
                * just set it to used, no need to do carve up the candidate node..
                */
                new_node = head;
                new_node->used = TRUE;
                ret = EXIT_SUCCESS;
            }
            break;
        } /* else ret = EXIT_FAILURE */
        head = head->next;
    }

    /* return a pointer to the new (or changed) node, on failure this will be NULL and ret==EXIT_FAILURE */
    *node_p = new_node;
    return ret;
}

/*
 * This routine will deallocate a given memory object (node).
 * It will group the newly unused node with any adjacent unallocated nodes to
 * form one new node, freeing as needed.
 *
 * ragasm obj is assumed to be already unloaded/deallocated.
 * When grouping the newly deallocated node with adjacent unused nodes,
 * we only have to worry about the deallocation
 * candidate node beacause the other nodes have already beeen removed.
 * memory object does not know about the base address of the vm area
 * it is mapped into, therefore we pass it in as a parameter.
 */
int
memory_dealloc(secmem_obj_t *node)
{
    int ret = EXIT_FAILURE;
    secmem_obj_t *tmp_node = node, *prev_node = node->prev , *next_node = node->next;

    /* group with previous node, ony if not used */
    if (prev_node && !prev_node->used) {
        prev_node->size += node->size;
        prev_node->next  = node->next;
        node = prev_node;
    }

    /* group with next node, ony if not used */
    if (next_node && !next_node->used) {
        node->size += next_node->size;
        if (next_node->next)
            next_node->next->prev = node;
        /* this relation is used as a predicate below to determine
         * whether or not we need to free the next node when we are finished
         * */
        node->next = next_node->next;
    }

    /* Test if the original node was grouped into the previous node */
    if (node == prev_node)
        free(tmp_node);
    /* Test if the next node was grouped into the  original/prev node*/
    if (next_node && next_node->next == node->next)
        free(next_node);

    node->used = FALSE;
    ret = EXIT_SUCCESS;
    return ret;
}

/*
 * Reading is carried out at the layer above (process), these routines are
 * stubs in case needed in the future.
 */
int
memory_read(SECMEM_INTERNAL_T addr, char *buffer, secmem_obj_t *head)
{
    int ret = EXIT_FAILURE;
    return ret;
}

/*
 * Writing is carried out at the layer above (process), these routines are
 * stubs in case needed in the future.
 */
int
memory_write(SECMEM_INTERNAL_T addr, secmem_obj_t *head)
{
    int ret = EXIT_FAILURE;
    return ret;
}

/* Get a memory object associated with a secmem vm address*/
int
memory_get_obj_by_addr(SECMEM_INTERNAL_T addr, secmem_obj_t  **node_p, secmem_obj_t *head)
{
    int ret = EXIT_FAILURE;
    while (head && head->offset <= addr) {
        if (head->offset == addr) {
            *node_p = head;
            ret = EXIT_SUCCESS;
            break;
        }
        head = head->next;
    }
    return ret;
}