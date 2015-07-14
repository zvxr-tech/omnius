/* omnius/process.c
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 * All of the routines assume:
 *      the type secmem_process_t input parameter is caller allocated,
 *      the type blob_t input parameter is already popululated with the incoming message blob.
 *
 * The incoming blob is used and modified to become the reply blob.
 *
 * All of these routines return EXIT_SUCCESS or EXIT_FAILURE.
 *
 * 2015 - Mike Clark
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "global.h"
#include "fsm_descriptor.h"
#include "process.h"

/*
 * Allocate space for, and generate FSM descriptors for the policies of a process being loaded.
 * POLICY is a pointer to series of policy_t objects laid out contingously in memory.
 * Because the policy objects themselves are variable length, it is important that the NEXT_POLICY macro is used
 * to iterate over them.
 *
 * Upon failure an attempt is made to unload all of the descriptors thus far, and free the allocated memory.
 *
 *
 * PARAMETERS
 * policy - a pointer to an array of policy objects. Each policy object is a policy to load. Each policy object is
 * variable length and neither it nor the policy array are NULL terminted.
 *
 * policy_count - number of policy objects pointed to by POLICY.
 *
 * proc - caller allocated, process object.
 *
 * RETURN
 * EXIT_SUCCESS on success, otherwise failure.
 *
 * NOTES
 * This routine allocates a enough room to store one fsm descriptor for each policy in the list provided as input.
 *  FSM are capable of large input alphabets, however we want to constrain ours to {'R','W'}
 */
int
process_load_fsm(policy_t *policy, SECMEM_INTERNAL_T policy_count, secmem_process_t *proc)
{
    int i, k, ret = EXIT_FAILURE;
    /* Allocate space for all of the fsm_descriptors. */
    proc->fsm_desc = (fsm_descriptor_t *) calloc(policy_count, sizeof(fsm_descriptor_t));
    if (proc->fsm_desc) {
        for (i = 0; i < policy_count; i++) {

            /* we can statically allocate the fsm_desc because we will be copying it by value to an entry
             * in the fsm_desc array field of the process instance and it does not need to persist beyond that.
             */
            fsm_descriptor_t fsm_desc;
            memset(&fsm_desc,0,sizeof(fsm_desc));
            ret = fsm_descriptor_load(policy->body.regex, policy->head.len, &fsm_desc);
            if (ret != EXIT_SUCCESS) {
                /* Upon failure, we need to unload all of the policies successfully loaded thus far */
                for (k = i - 1; k >= 0; k--) {
                    if (fsm_descriptor_unload(&proc->fsm_desc[k]) == EXIT_FAILURE)
                        k = k; // TODO RAISE AN ALARM
                }
                free(proc->fsm_desc);
                break;
            } else {
                proc->fsm_desc[i] = fsm_desc;
            }
            policy = NEXT_POLICY(policy);
        }
    }
    return ret;
}


/* Unload the FSM descriptors associated with a process. */
int
process_unload_fsm(secmem_process_t *proc)
{
    int i, ret = EXIT_SUCCESS;
    for (i = 0; i < proc->fsm_count; i++) {
        if (fsm_descriptor_unload(&proc->fsm_desc[i]) == EXIT_FAILURE) {
            i = i; // TODO Alert to prevent memory leaks
            ret = EXIT_FAILURE;
        }
    }
    free(proc->fsm_desc);
    return ret;
}

/* Load a process object for a process that will use omnius' services */
int
process_load(blob_t *blob, secmem_process_t *proc)
{
    int ret = EXIT_FAILURE;

    /* allocate the entire region of secure memory for this process */
    if ((proc->base = (char *) calloc(blob->head.size, sizeof(char))) != NULL) {
            if (memory_load(blob->head.size, &proc->secmem_head) == EXIT_SUCCESS) {
                proc->pid = blob->head.pid;
                proc->mem_size = blob->head.size;
                proc->fsm_count = blob->head.policy_count;
                ret = process_load_fsm(blob->body.policy_entry, blob->head.policy_count, proc);
            }
    }

    /*
     * Upon failure, we must undo what has been done. This includes, calling undo swhat ubfunctions have done and
     * deallocating memory allocations made during this routine. We don't call process_unload because that makes certain
     * assumptions about the state of the secmem_process object.
     */
    if (ret != EXIT_SUCCESS) {
        if (proc->secmem_head) {
            memory_unload(proc->secmem_head);
        }
        if (proc->base)
            free(proc->base);
    }

    /* REPLY */
    blob->head.data_len = 0;

    return ret;
}

/* Unload a process, as specified in a blob message.
 *
 * This will attempt to completely unload a process object representing a target process using omnius' service.
 * If this returns EXIT_FAILURE, no guarantees can be made regarding the complete deallocation of dynamically
 * allocated instances associated with the process; this may lead to a memory leak.
 *
 */
int
process_unload(blob_t *blob, secmem_process_t *proc)
{
    int ret = EXIT_FAILURE;
    /*
     * We use |= to retain any non-zero (failure) return codes, because we want to continue everything even if
     * one step fails only works because a success is zero and failure is non-zero
     * Freeing secmem_head is done by the memory_unload function
     */
    ret = memory_unload(proc->secmem_head);
    ret |= process_unload_fsm(proc);

    /* Zero out the the entire memory region associated with this process */
    if (proc->base) {
        memset(proc->base, 0, proc->mem_size);
        free(proc->base);
    }

    /* REPLY */
    blob->head.data_len = 0;

    return ret;
}

/* Allocate a memory object associated with a secmem vm address for a given process, as specified in a blob message */
int
process_alloc(blob_t *blob, secmem_process_t *proc) {
    int ret = EXIT_FAILURE;

    /* pull up the FSM description using the policy_id specified */
    fsm_descriptor_t *fsm_desc = &proc->fsm_desc[blob->head.policy_id];

    /* secmem address allocated */
    secmem_obj_t *new_node;

    /* allocate memory object, and then load a ragasm object to manage it with respect to secmem */
    if (memory_alloc(blob->head.size, fsm_desc, &new_node, &proc->secmem_head) == EXIT_SUCCESS) {
        /* zero out the memory, just in case the dealloc failed to clear it.
         * Since you can only read allocated nodes, and allocated nodes are guaranteed to be zero'd out,
         * you cannot read residual data in memory.
         */
        assert(proc->base);
        memset(proc->base + new_node->offset, 0, new_node->size);
        if ((ret = ragasm_load(fsm_desc, &new_node->ragasm)) != EXIT_SUCCESS)
            memory_dealloc(new_node); // TODO raise an alarm if this fails.
    }

    /* REPLY */
    /*set the address field of the reply blob if the allocation was successful */
    blob->head.addr = ret == EXIT_SUCCESS ? new_node->offset : 0;
    blob->head.data_len = 0;

    return ret;
}

/* Deallocate and zero out the value associated with a memory object associated with a secmem vm address for a given
 * process, as specified in a blob message
 */
int
process_dealloc(blob_t *blob, secmem_process_t *proc)
{
    int ret = EXIT_FAILURE;

    /* get a reference to the memory object associated with the (secmem vm) address and test if it is in use before
     * coninuing deallocation
     */
    secmem_obj_t *node;
    if ((memory_get_obj_by_addr(blob->head.addr, &node, proc->secmem_head)) == EXIT_SUCCESS && node->used) {
        /*
         * Zero out the memory, then deallocate the ragasm, before the memory object. If either fails, the other action should still be
         * attempted while preserving any non-zero return values (error) by logical OR'ing.
         */
        if (proc->base)
            memset(proc->base + node->offset, 0, node->size);
        ret = ragasm_unload(&node->ragasm);
        ret |= memory_dealloc(node);

    }

    /* REPLY */
    blob->head.data_len = 0;
    return ret;
}

/* Read N bytes beginning from a process (PROC) secmem vm address as specified in the blob's data_len and addr field,
 * respectively. The data is read into the data field of the blob which is the reply.
 */
int
process_read(blob_t *blob, secmem_process_t *proc )
{
    int ret = EXIT_FAILURE;

    /*  get a reference to the memory object */
    secmem_obj_t *node;
    /* only allow access to memory that has already been allocated */
    if (((memory_get_obj_by_addr(blob->head.addr, &node, proc->secmem_head)) == EXIT_SUCCESS && node->used) &&
            node->used &&
            node->ragasm.is_loaded) {
        /* check that the size requested is within the bounds of the memory object @ addr */
        if (blob->head.data_len <= node->size) {
            ragasm_step(node->ragasm.fsm_desc->alpha_map[READ_CHAR], &node->ragasm);
            ret = ragasm_validate(&node->ragasm);
            if (ret == EXIT_SUCCESS) {
                /* copy data out to reply blob */
                void *src = (void *) (proc->base + node->offset);
                void *dst = blob->body.data;
                memmove(dst, src, blob->head.data_len);
            }
        }
    }

    if (ret != EXIT_SUCCESS)
        blob->head.data_len = 0;
    return ret;
}

/*  Write N bytes beginning at a process (PROC) secmem vm address as specified in the blob's data_len and addr field,
 * respectively. The data is written from the data field of the incoming request blob.
 */
int
process_write(blob_t *blob, secmem_process_t *proc)
{
    int ret = EXIT_FAILURE;

    /*  get a reference to the memory object */
    secmem_obj_t *node;
    /* only allow access to memory that has already been allocated */
    if (((memory_get_obj_by_addr(blob->head.addr, &node, proc->secmem_head)) == EXIT_SUCCESS && node->used) &&
        node->used &&
        node->ragasm.is_loaded) {
        /* check that the size requested is within the bounds of the memory object @ addr */
        if (blob->head.data_len <= node->size) {
            ragasm_step(node->ragasm.fsm_desc->alpha_map[WRITE_CHAR], &node->ragasm);
            ret = ragasm_validate(&node->ragasm);
            if (ret == EXIT_SUCCESS) {
                /* copy data out to reply blob */
                void *src = (void *) blob->body.data;
                void *dst = (void *) (proc->base + node->offset);
                memmove(dst, src, blob->head.data_len);
            }
        }
    }

    blob->head.data_len = 0; /* reply shall have no data */
    return ret;
}