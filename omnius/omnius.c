/* omnius/omnius.c
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 *
 * Only the reply blobs from a read action will have the data field populated.
 *
 * 2015 - Mike Clark
 */
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <stdio.h>
#include <errno.h>
#include "global.h"
#include "omnius.h"
#include "process.h"
#include "comm.h"



FILE *g_logfile;

/* These strings are used to easilly identify the bit-mode that omnius is operating in,
 */
#if SECMEM_INTERNAL_BIT == 32
char g_bit_mode_str[] = "32bit-mode";
#elif SECMEM_INTERNAL_BIT == 64
char g_bit_mode_str[] = "64bit-mode";
#else
char g_bit_mode_str[] = "<ERROR>";
#endif

/* Global table containing pointers to the data structures used to manage
 * each processes secure memroy.
 *
 * Indexed by process_id.
 */
secmem_process_t *g_pid_lookup[MAX_PID];

/* Global dispatch table, we use a table instead of a giant switch for readability and maintainability of the code
 * that executes the handelers assocaited with each request type.
 */
int (*g_dispatch[MTYPE_COUNT]) (blob_t *);

/* Catch user interrupt and shutdown gracefully */
struct sigaction g_int_act, g_int_oldact;

/*
 * This is the entry point for loading (registering) a process with omnius.
 */
int
omnius_load(blob_t *blob) {
    int ret = EXIT_FAILURE;

    if (!g_pid_lookup[blob->head.pid]) {
         /* create a proc based on pid */
        secmem_process_t *proc = (secmem_process_t *) calloc(1, sizeof(secmem_process_t));
        if (proc) {
            if ((ret = process_load(blob, proc)) == EXIT_SUCCESS) {
                /* If we are successful, add the process object to a global lookup table for future reference, otherwise free mem */
                g_pid_lookup[blob->head.pid] = proc;
            } else {
                free(proc);
            }
        }
    }

    return ret;
}

/*
 * This is the entry point for unloading a process with omnius.
 */
int
omnius_unload(blob_t *blob) {
    int ret = EXIT_FAILURE;

    /* find the proc based on pid */
    secmem_process_t *proc = g_pid_lookup[blob->head.pid];
    if (proc) {
        ret = process_unload(blob, proc);
        free(proc);
    }

    g_pid_lookup[blob->head.pid] = NULL;

    return ret;
}

/*
 * This is the entry point for allocating memory for an process already loaded into omnius.
 */
int
omnius_alloc(blob_t *blob) {
    int ret = EXIT_FAILURE;

    /* find the proc based on pid */
    secmem_process_t *proc = g_pid_lookup[blob->head.pid];

    /* validate and process*/
    if (proc &&  blob->head.size >= 0 && blob->head.size < proc->mem_size && blob->head.policy_id < proc->fsm_count) {
        /* assuming success, the process routine will stuff the allocation address into blob */
        ret = process_alloc(blob, proc);
    }
    return ret;
}

/*
 *  This is the entry point for deallocating secure memory (secmem).
 */
int
omnius_dealloc(blob_t *blob) {
    int ret = EXIT_FAILURE;

    /* find the proc based on pid */
    secmem_process_t *proc = g_pid_lookup[blob->head.pid];

    /* validate and process*/
    if (proc &&  blob->head.addr >= 0 && blob->head.addr < proc->mem_size) {
        ret = process_dealloc(blob, proc);
    }

    return ret;
}

/*
 *  This is the entry point for reading data from a secure memory.
 */
int
omnius_read(blob_t *blob) {
    int ret = EXIT_FAILURE;

    /* find the proc based on pid */
    secmem_process_t *proc = g_pid_lookup[blob->head.pid];

    /* validate and process*/
    if (proc &&  blob->head.addr >= 0 && blob->head.addr < proc->mem_size && blob->head.data_len > 0) {
        /* assuming success, the process routine will stuff the data read into the data field of the blob
         * */
        ret = process_read(blob, proc);
    }
    return ret;
}

/*
 *  This is the entry point for reading data from a secure memory.
 */
int
omnius_write(blob_t *blob) {
    int ret = EXIT_FAILURE;

    /* find the proc based on pid */
    secmem_process_t *proc = g_pid_lookup[blob->head.pid];

    /* validate */
    if (proc &&  blob->head.addr >= 0 && blob->head.addr < proc->mem_size && blob->head.data_len > 0) {
        ret = process_write(blob, proc);
    }
    return ret;
}


/* DO NOT USE  YET
 * This will output numeric encoding (according to a simple schema) of omnius statistics, that can be
 * easilly parsed by programs like awk to generate custom views of the stats omnius reports.
 *
 *
 * FORMAT
 * ------
 * pid, FS
 * total_mem_size, FS
 * mem_usage, FS
 * policy_count, FS
 *   regex, FS
 *   usage (refcount), FS
 *      ...
 *  (memory)
 *      used/free, FS
 *      start, FS
 *      finish, FS
 *      size, FS
 *      policy, FS
 *      policy_state, RS
 *      ...
 * RS
 *
 *
 */
int
omnius_view(blob_t *blob) {
    int ret = EXIT_FAILURE;
    size_t len = 0;


    /* find the proc based on pid */
    secmem_process_t *proc = g_pid_lookup[blob->head.pid];
    void *buffer_p = blob->body.data; /* use the entire blob area */
    /* validate */
    if (proc) {
        /* pid, mem_size */
        len += snprintf(blob->body.data + len, MAX_BLOB_DATA_SIZE - len, "%d%c%zu%c", proc->pid, FS, (size_t) proc->mem_size, FS);

        /* policies */
        len += snprintf(blob->body.data + len, MAX_BLOB_DATA_SIZE - len, "%zu%c%", (size_t) proc->fsm_count, FS);
        for (int i = 0; i < proc->fsm_count; i++, buffer_p = blob + len)
            len += snprintf(blob->body.data + len, MAX_BLOB_DATA_SIZE - len, "%s%c%zu%c", proc->fsm_desc[i].comment, FS,
                            (size_t) proc->fsm_desc->ref_count), FS;
        /* memory */
        secmem_obj_t *head = proc->secmem_head;
        while (head) {
            len += snprintf(blob->body.data + len, MAX_BLOB_DATA_SIZE - len, "%d%c%zu%c%zu%c%zu%c%s%c%zu%c",
                            head->used, FS,
                            (size_t) head->offset, FS,
                            (size_t) (head->offset + head->size - 1), FS,
                            (size_t) head->size, head->ragasm.fsm_desc->comment, FS,
                            (size_t) head->ragasm.curr_state), RS;
            head = head->next;
        }
        len += snprintf(blob->body.data + len, MAX_BLOB_DATA_SIZE - len, "%c", RS);
    }
    if (len > 0)
        ret = EXIT_SUCCESS;
    return ret;
}

/*
 * This will print statistics about omnius to stdout.
 */
int
omnius_view_internal(blob_t *blob) {
    int ret = EXIT_FAILURE;
    size_t len = 0;


    /* find the proc based on pid */
    secmem_process_t *proc = g_pid_lookup[blob->head.pid];
    void *buffer_p = blob->body.data; /* use the entire blob area */
    /* validate */
    if (proc) {
        /* pid, mem_size */
        printf("PID:\t%d\n\tTotal Size: 0x%x\n", proc->pid, proc->mem_size);
        for (int i = 0; i < proc->fsm_count; i++, buffer_p = blob + len)
            printf("\tPolicy: %d\n\t\tRegex: %s\n\t\tRef Count: %zu\n", i, proc->fsm_desc[i].comment, proc->fsm_desc[i].ref_count);
        /* memory */
        printf("\tMemory:\n\tstart\tend\tsize\tused\tregex\tstate\n");
        secmem_obj_t *head = proc->secmem_head;
        while(head) {
            printf("\t0x%x\t0x%x\t0x%x\t", head->offset, head->offset + head->size - 1, head->size);
            printf("%c\t%s\t%zu\n", head->used ? 'X' : ' ', (head->ragasm.fsm_desc && head->ragasm.fsm_desc->comment)? head->ragasm.fsm_desc->comment : "" , head->ragasm.curr_state);
            head = head->next;
        }
        printf("\n");
        ret = EXIT_SUCCESS;
    }

    return ret;
}


/*
 * Dummy message type.
 * This can be used to test basic connectivity. (Or DOSing)
 */
int
omnius_nil(blob_t *blob)
{
    return EXIT_SUCCESS;
}


/*
 * Listen for incoming messages on the ipc_in message queue.
 *
 *
 * msgbuf_t instances mtext field are of fixed size (MAX_MTEXT_SIZE)
 */

int
listen(int ipc_in, int ipc_out)
{
    msgbuf_t msg_buf;
    char log_buf[MAX_HUMANIZE_LEN];
    int ret = EXIT_FAILURE;

    //msgbuf_t msg_request, msg_reply;
    //memset(&msg_request, 0, sizeof(msg_request));
    //memset(&msg_reply, 0, sizeof(msg_reply));

    while (1) {
        /* ZERO out the buffer */
        memset(&msg_buf, 0, sizeof(msg_buf));

        /* LISTEN */
        if ((msgrcv(ipc_in, &msg_buf, MAX_MTEXT_SIZE, 0, 0) == -1) && (SIZEOF_BLOB(&msg_buf.blob) <= MAX_MTEXT_SIZE)) {
            perror("msgrcv");
            continue;
        }
        humanize_blob(&msg_buf, log_buf, sizeof(log_buf));
        fprintf(g_logfile, "%s", log_buf);

        /*
         * Exit upon receipt of the termination message.
         */
        if (msg_buf.mtype == MTYPE_TERMINATE) {
            ret = OMNIUS_RET_SUCCESS;
            break;
        }

        /* It is crucial that we bounds check the mtype because we are using it to index into a fixed
         * sized array -- i.e. overflow.
         */
        if (msg_buf.mtype >= 0 && msg_buf.mtype < MTYPE_COUNT && msg_buf.blob.head.pid < MAX_PID) {

            /* ACTION */
            ret = g_dispatch[msg_buf.mtype](&msg_buf.blob);
            if (ret != EXIT_SUCCESS ) {
                msg_buf.blob.head.data_len = 0;
            }
            /*
             * Change message-type depending on the success of the event and
             * calculate the length of the entire message buffer used.
             */
            msg_buf.mtype =  msg_buf.mtype | (ret == EXIT_SUCCESS ? MTYPE_MOD_ACK : MTYPE_MOD_NAK);

            if ((SIZEOF_BLOB(&msg_buf.blob) > MAX_MTEXT_SIZE) || ((ret = msgsnd(ipc_out, &msg_buf, SIZEOF_BLOB(&msg_buf.blob), 0)) == -1))
                perror("msgsnd");
        } else {
            msg_buf.mtype |= MTYPE_MOD_NAK; /* set nak-reply type if request was invalid */
        }
        humanize_blob(&msg_buf, log_buf, sizeof(log_buf));
        fprintf(g_logfile, "%s", log_buf);
    }
    return ret;
}



/*
 * This routine handles sigterm signals so that the program can be shutdown gracefully.
 */
void
sigint_handler(int signum)
{
    printf("\tCaught SIGINT\n");
    shutdown();
    exit(EXIT_FAILURE);
}

/*
 * This routine sets up a signal handler to catch SIGINT and preserves the old handler for eventual re-activation after
 * this program terminates.
 */
int
setup_sigint(void)
{
    memset(&g_int_act, 0, sizeof (g_int_act));
    memset(&g_int_oldact, 0, sizeof (g_int_oldact));
    g_int_act.sa_handler = sigint_handler;
    sigaddset(&g_int_act.sa_mask, SIGINT);
    g_int_act.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &g_int_act, &g_int_oldact) == 0)
        return EXIT_SUCCESS;

    perror("Error registering signal handler.\n");
    return EXIT_FAILURE;
}


/*
 * Run once on startup.
 */
int
startup(char **argv, int *msg_in, int *msg_out)
{
    key_t msg_in_key, msg_out_key;
    printf("Configuring...");

    /* Setup logging */
    g_logfile = stdout;

    /* register interrupt handler */
    if (setup_sigint() != EXIT_SUCCESS)
        return OMNIUS_RET_SIGNAL;

    /* clear the pid lookup table, because we interpret null=0 as empty. */
    memset(g_pid_lookup, 0, sizeof(g_pid_lookup));

    /* Clear the message queue */
    /* TODO future...maybe... *if* it should */

    /*
     * Populate the message action dispatch table
     * These are the routines to call depending on the message mtype.
     */
    g_dispatch[MTYPE_NIL] 		= omnius_nil;
    g_dispatch[MTYPE_LOAD] 		= omnius_load;
    g_dispatch[MTYPE_UNLOAD] 	= omnius_unload;
    g_dispatch[MTYPE_ALLOC] 	= omnius_alloc;
    g_dispatch[MTYPE_DEALLOC] 	= omnius_dealloc;
    g_dispatch[MTYPE_READ] 		= omnius_read;
    g_dispatch[MTYPE_WRITE] 	= omnius_write;
    g_dispatch[MTYPE_VIEW]	 	= omnius_view_internal; /* omnius_view; */
    g_dispatch[MTYPE_TERMINATE] = omnius_nil;

    printf("Starting OMNIUS in %s...\n", g_bit_mode_str);
    /* Parse command arguments */
    if(*msg_in)
        msg_in_key = *msg_in;
    else
        msg_in_key = (int) strtol(argv[1], NULL, 16);

    if(*msg_out)
        msg_out_key = *msg_out;
    else
        msg_out_key = (int) strtol(argv[2], NULL, 16);

    if (msg_out_key == msg_in_key)
        return OMNIUS_RET_ARGS;

    /* connect to the outgoing msg queue, then incoming queue */
    if (((*msg_out = ipc_connect((key_t) msg_out_key)) < 0) ||
        ((*msg_in = ipc_connect((key_t) msg_in_key)) < 0))
        return OMNIUS_RET_MSG;

    printf("Loaded!\n");
    return EXIT_SUCCESS;
}

/*
 * Called once on termination.
 */
void
shutdown(void)
{
    printf("Terminating....");
    if (g_logfile)
    {
        fflush(g_logfile);
        fclose(g_logfile);
    }
    printf("Done.\n");
    fflush(stdout);
    return;
}


/*
 * Show program usage.
 */
void
show_usage(int ret)
{
    fprintf(stderr, "\nERR:%d\nUsage: omnius [msg_in_key] [msg_out_key]\n", ret);
    return;
}

/*
 * Entry for OMNIUS
 */
int
main(int argc, char **argv) {
    int ret = EXIT_FAILURE, msg_in = 0, msg_out = 0;

    /* Parse cmd-args and setup globals */
    if (argc < 3) {
        /* if these are set, startup() will not try and parse cmd args for in/out msgids */
        msg_in = 0xdead1;
        msg_out = 0xdead2;
    }

    if ((ret = startup(argv, &msg_in, &msg_out)) == EXIT_SUCCESS) {
        /* start listening for messages */
        ret = listen(msg_in, msg_out);

        /* cleanup and exit */
        shutdown();
        ret = EXIT_SUCCESS;
    } else {
        show_usage(ret);
        ret = EXIT_FAILURE;
    }
    return ret;
}