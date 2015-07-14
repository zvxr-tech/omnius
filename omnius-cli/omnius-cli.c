/* omnius-cli/omnius-cli.c
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 *
 * Command-Line Interface to talk with omnius via IPC message queue interface..
 *
 * 2015 - Mike Clark
 */

#include <stdio.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <string.h>
#include "../omnius/global.h"
#include "omnius-cli.h"
#include "../omnius/memory.h"
#include "../omnius/omnius.h"



FILE *g_logfile;

/* These strings are used to easilly identify the bit-mode that omnius-cli is operating in,
 */
#if SECMEM_INTERNAL_BIT == 32
char g_bit_mode_str[] = "32bit-mode";
#elif SECMEM_INTERNAL_BIT == 64
char g_bit_mode_str[] = "64bit-mode";
#else
char g_bit_mode_str[] = "<ERROR>";
#endif

/* Catch user interrupt to cancel input */
struct sigaction g_int_act, g_int_oldact;

/* SCAN_MTYPE ROUTINES
 *
 * All of the scan_mtype_* routines expect a pointer to an allocated, initialized (to zero) blob instance,
 * generate a message from stdin, and return EXIT_SUCCESS or EXIT_FAILURE.
 *
 */

int
scan_mtype_load(blob_t *blob)
{

    // Field 1 (pid)
    printf("PID (0 to return):");
    while (fscanf(stdin, "%d", &blob->head.pid) < 1) {;}
    if (blob->head.pid == 0)
        return EXIT_FAILURE; /* error */

    // Field 2 - mem_size
    printf("Total memory size (0 to return): 0x");
    while (fscanf(stdin, "%x", &blob->head.size) < 1) {;}
    if (blob->head.size == 0)
        return EXIT_FAILURE; /* error */

    // Field 3 - policy count
    printf("Number of policy (0 to return):");
    while (fscanf(stdin, "%zu", (size_t *)&blob->head.policy_count) < 1) {;}
    if (blob->head.policy_count == 0)
        return EXIT_FAILURE; /* error */

    /* Field 4 - Set below */




    /*
     * For each policy declared we need space for one policy head plus the length of the policy body.
     */
    SECMEM_INTERNAL_T data_len = 0;
    policy_t *policy_entry = blob->body.policy_entry ;
    for (int i = 0; i < blob->head.policy_count; i++) {
        printf("Policy #%d\n", i+1);

        /* Policy - symbol count */
        printf("Symbol count for Number of policy (0 to return):");
        while (fscanf(stdin, "%zu", (size_t *)&policy_entry->head.symbol_count) < 1) {;}
        if (policy_entry->head.symbol_count == 0)
            return EXIT_FAILURE; /* error */

        /* Policy - regex string*/
        char regex[MAX_MTEXT_SIZE]; /* The regex can never be larger than this to transmit successfully */
        printf("Regex ([ESC][ENTER] to return):");
        while (fscanf(stdin, "%s", regex) < 1) {;}
        size_t regex_len = strlen(regex);
        if (regex[0] == ESC_CHAR ||
                regex_len < 1) {
            return EXIT_FAILURE; /* error */
        }
        policy_entry->head.len = (SECMEM_INTERNAL_T) regex_len;

        /* check if there is enough room in the underlying data struct instance to store this regex.
         * If so, copy it.
         */
        if (((MAX_SECMEM_INTERNAL - data_len)- sizeof(policy_head_t)) < regex_len) {
            /* Not enough room left to store the size of the blob data. */
            return EXIT_FAILURE; /* error */
        }
        memcpy(policy_entry->body.regex, regex, policy_entry->head.len);

        /* increment the write pointer (policy_entry), we need to manually calculate this because the struct declares the policy_entry
         * field as an array of length 1. We are casting this over a larger (MAX_MTEXT_DATA) instance and use this field
         * as an actual array, but with members who are variable length because of the regex
         */
        /* policy_len = head len + body_len */
        size_t policy_len = SIZEOF_POLICY(policy_entry); //sizeof(policy_head_t) + policy_entry->head.len;
        data_len += policy_len;
        //policy_entry = (policy_t *) ((char *)policy_entry + policy_len);
        policy_entry = NEXT_POLICY(policy_entry) ;
    }
    blob->head.data_len = data_len;
    return EXIT_SUCCESS;
}

int
scan_mtype_unload(blob_t *blob)
{

    /*  Field 1 (pid) */
    printf("PID (0 to return):");
    while (fscanf(stdin, "%d", &blob->head.pid) < 1) {;}
    if (blob->head.pid == 0)
        return EXIT_FAILURE; /*  error */

    /*  Field 2 - EMPTY */
    /*  Field 3 - EMPTY */
    /*  Field 4 - EMPTY */
    /*  Field 5 - EMPTY */

    return EXIT_SUCCESS;
}

int
scan_mtype_alloc(blob_t *blob)
{

    /*  Field 1 - pid */
    printf("PID (0 to return):");
    while (fscanf(stdin, "%d", &blob->head.pid) < 1) {;}
    if (blob->head.pid == 0)
        return EXIT_FAILURE; /*  error */

    /*  Field 2 - size of memory object */
    printf("Memory size (0 to return): 0x");
    while (fscanf(stdin, "%x", &blob->head.size) < 1) {;}
    if (blob->head.size == 0)
        return EXIT_FAILURE; /*  error */

    /*  Field 3 - policy to apply to the memory object */
    printf("Policy id (0 to return):");
    while (fscanf(stdin, "%zu", (size_t *)&blob->head.policy_id) < 1) {;} /*  since 0 is a valid policy number we use zero-length input to escape */
    if (blob->head.policy_id == 0)
        return EXIT_FAILURE; /*  error */
    blob->head.policy_id--; /*  re-index from zero */

    /*  Field 4 - EMPTY */
    /*  Field 5 - EMPTY */

    return EXIT_SUCCESS;
}

int
scan_mtype_dealloc(blob_t *blob)
{
    /*  Field 1 - pid */
    printf("PID (0 to return):");
    while (fscanf(stdin, "%d", &blob->head.pid) < 1) {;}
    if (blob->head.pid == 0)
        return EXIT_FAILURE; /* error */

    /*  Field 2 - memory address */
    printf("Secure-Memory address ([ESC][ENTER] to return):");
    while (fscanf(stdin, "%x", &blob->head.addr) < 1) {;}
    if (blob->head.addr == ESC_CHAR)
        return EXIT_FAILURE; /*  error */

    /*  Field 3 - EMPTY */
    /*  Field 4 - EMPTY */
    /*  Field 5 - EMPTY */

    return EXIT_SUCCESS;

}

int
scan_mtype_read(blob_t *blob)
{
    /*  Field 1 - pid */
    printf("PID (0 to return):");
    while (fscanf(stdin, "%d", &blob->head.pid) < 1) {;}
    if (blob->head.pid == 0)
        return EXIT_FAILURE; /* error */

    /*  Field 2 - secmem address to read from */
    printf("Address to read from ([ESC][ENTER] to return): 0x");
    while(fscanf(stdin, "%x", &blob->head.addr) < 1) {;}
    if (blob->head.addr == ESC_CHAR)
        return EXIT_FAILURE; /* error */

    /*  Field 3 - EMPTY */
    /*  Field 4 - EMPTY */

    /*  Field 5 - How much data to read  */
    printf("Read length (Bytes) ([ESC][ENTER] to return): 0x");
    while(fscanf(stdin, "%x", &blob->head.data_len) < 1) {;}
    if (blob->head.data_len == 0)
        return EXIT_FAILURE; /* error */

    return EXIT_SUCCESS;


}

int
scan_mtype_write(blob_t *blob)
{

    /*  Field 1 - pid */
    printf("PID (0 to return):");
    while (fscanf(stdin, "%d", &blob->head.pid) < 1) {;}
    if (blob->head.pid == 0)
        return EXIT_FAILURE; /*  error */


    /*  Field 2 - secmem address to write to */
    printf("Address to write to ([ESC][ENTER] to return): 0x");
    while(fscanf(stdin, "%x", &blob->head.addr) < 1) {;}
    if (blob->head.addr == ESC_CHAR)
        return EXIT_FAILURE; /* error */

    /*  Field 3 - EMPTY */

    /*  Field 4 - Set below */

    /*  Field 5 - data to write */
    char *lbuf = (char *) &blob->body.data; /*  remember, there is statically allocated buffer backing this. live dangerously, write directly to it. */

    printf("Enter data. (ASCII -> RAW. ie. \"10\" => 0x10) (NaN to return):");
    unsigned int raw_num;

    /* TODO change to multi-byte input */
    while (fscanf(stdin, "%x", &raw_num)) {;}
    char *raw_ch = (char *) &raw_num; /*  cast so that byte oredering does not matter when reading off a single byte. */
    int k;
    for (k = 0; k < (sizeof(raw_num) / sizeof(char)); k++) {
        memcpy(lbuf++, &raw_ch[k], sizeof(char));
    }

    /*  Field 4 - bytes to read (expected) */
    blob->head.data_len = sizeof(raw_num);

    return EXIT_SUCCESS;
}

int
scan_mtype_view(blob_t *blob)
{
    /*  Field 1 - pid */
    printf("PID (0 to return):");
    while (fscanf(stdin, "%d", &blob->head.pid) < 1) {;}
    if (blob->head.pid == 0)
        return EXIT_FAILURE; /* error */
    /*  Field 2 - EMPTY */
    /*  Field 3 - EMPTY */
    /*  Field 4 - EMPTY */
    /*  Field 5 - EMPTY */
    return EXIT_SUCCESS;
}
/* Send a hardcoded test load message. */
int
scan_mtype_test(blob_t *blob)
{
    blob->head.pid = 22;
    blob->head.size = 100;
    blob->head.policy_count = 1;

    blob->body.policy_entry[0].head.symbol_count = 2;
    blob->body.policy_entry[0].head.len = 8;
    blob->body.policy_entry[0].body.regex[0] = '(';
    blob->body.policy_entry[0].body.regex[1] = 'R';
    blob->body.policy_entry[0].body.regex[2] = 'W';
    blob->body.policy_entry[0].body.regex[3] = 'R';
    blob->body.policy_entry[0].body.regex[4] = '*';
    blob->body.policy_entry[0].body.regex[5] = 'W';
    blob->body.policy_entry[0].body.regex[6] = ')';
    blob->body.policy_entry[0].body.regex[7] = '*';

    /*
     * For each policy declared we need space for one policy head plus the length of the policy body.
     */
    SECMEM_INTERNAL_T data_len = 0;
    int ret = 0;
    for (int i = 0; i < blob->head.policy_count; i++) {
        if (((MAX_SECMEM_INTERNAL - data_len)- sizeof(policy_head_t)) < blob->body.policy_entry[i].head.len) {
            /* Not enough room left to store the size of the blob data. */
            ret = 1;
            break;
        }
        data_len += sizeof(policy_head_t); /* head len */
        data_len += blob->body.policy_entry[i].head.len; /* body len */
    }
    if (!ret) {
        blob->head.data_len = data_len;
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE; /* error */
    }

}

/* TODO WIP - for future use when omnius replies to view commands with a print buffer stuffed in the blob data field
 * Currently, view command causes omnius to print to it's own stdin, and transmits a simple ack response with no data
 */
void
print_view(msgbuf_t msgbuf) {
    pid_t pid, used;
    size_t policy_count, mem_size, ref_count, offset_start, offset_end, size, curr_state;
    char comment[MAX_FSM_COMMENT_LEN];
    void *buffer_p  = msgbuf.blob.body.data;

    /* pid, mem_size */
    buffer_p += sscanf(buffer_p, "%d%zu", pid, mem_size);
    printf("PID:\t%d\nTotal Mem:\t%zu\n", pid, mem_size);

    /* policies */
    buffer_p += sscanf(buffer_p, "%zu", policy_count);
    for (int i = 0; i < policy_count; i++) {
        buffer_p += sscanf(buffer_p, "%sFS%zuFS", comment, ref_count);
        printf("Policy:\t%d\nRegex:\t%s\nRef Count:\t%zu\n", i, comment, ref_count);
    }

    printf("used\tstart\tend\tsize\tregex\tstate\n");
    size_t len = 0;
    do { /* we are guaranteed at least one memory node */
        //buffer_p += sscanf(buffer_p, "%dFS%zuFS%zuFS%zuFS%sFS%zuRS\"",
        buffer_p += sscanf(buffer_p, "%d%zu%zu%zu%s%zu\"",
                           &used,
                           &offset_start,
                           &offset_end,
                           &size,
                           comment,
                           &curr_state);
        printf("%d\t%zu\t%zu\t%zu\t%s\t%zu\n",
               used,
               offset_start,
               offset_end,
               size,
               comment,
               curr_state);
        len += size;
    } while (len < mem_size);
    return;
}


/* Upon input from user through stdin, generate a message to send to omnius.
 * Upon send, listen for a reply and display the response.
 */
int
cli(int msgqid_in, int msgqid_out)
{
    char log_buf[MAX_HUMANIZE_LEN];

    int terminate = 0;
    while(!terminate)
    {
        char c[12];
        printf("\n(L)oad, (U)nload, (A)llocate, (D)eallocate, (R)ead, (W)rite, (V)iew, (T)erminate, (Q)uit (^C to cancel): ");
        while (fscanf(stdin, "%s", c) < 1) {;}
        *c &= 0xDF; /*  force uppercase */


        msgbuf_t in_buf, out_buf;
        memset(&in_buf, 0, sizeof(msgbuf_t));
        memset(&out_buf, 0, sizeof(msgbuf_t));
        size_t mtext_len = 0;
        int ret = EXIT_FAILURE;
        switch (*c)
        {
            case 'L': /*  load */
                in_buf.mtype = MTYPE_LOAD;
                ret = scan_mtype_load(&in_buf.blob);
                break;
            case 'U': /*  unload */
                in_buf.mtype = MTYPE_UNLOAD;
                ret = scan_mtype_unload(&in_buf.blob);
                break;
            case 'A': /*  allocate */
                in_buf.mtype = MTYPE_ALLOC;
                ret = scan_mtype_alloc(&in_buf.blob);
                break;
            case 'D': /*  deallocate */
                in_buf.mtype = MTYPE_DEALLOC;
                ret = scan_mtype_dealloc(&in_buf.blob);
                break;
            case 'W': /*  write */
                in_buf.mtype = MTYPE_WRITE;
                ret = scan_mtype_write(&in_buf.blob);
                break;
            case 'R': /*  read */
                in_buf.mtype = MTYPE_READ;
                ret = scan_mtype_read(&in_buf.blob);
                break;
            case 'V': /*  view (print memory to stdout on device where OMNIUS instance is executing) */
                in_buf.mtype = MTYPE_VIEW;
                ret = scan_mtype_view(&in_buf.blob);
                break;
            case 'T': /*  terminate OMNIUS */
                in_buf.mtype = MTYPE_TERMINATE;
                ret  = EXIT_SUCCESS;
                break;
            case 'X': /*  hard-coded, test load message */
                in_buf.mtype = MTYPE_LOAD;
                ret = scan_mtype_test(&in_buf.blob);
                break;
            case 'Q': /*  quit CLI */
                ++terminate;
                break;
            default:
                printf("Unknown command '%c'\n", *c);
                ret = EXIT_SUCCESS;
        }
        if (ret == EXIT_SUCCESS)
        {

            if ((SIZEOF_BLOB(&out_buf.blob) <= MAX_MTEXT_SIZE) && (msgsnd(msgqid_in, &in_buf, SIZEOF_BLOB(&in_buf.blob), 0) != -1)) {
                if ((msgrcv(msgqid_out, &out_buf, MAX_MTEXT_SIZE, 0, 0) != 1) && (SIZEOF_BLOB(&out_buf.blob) <= MAX_MTEXT_SIZE)) {
                    humanize_blob(&out_buf, log_buf, sizeof(log_buf));
                    fprintf(g_logfile, "%s", log_buf);
                }
                else
                    perror("msgrcv");
            }
            else
                perror("Error sending message");
        }
        else
            perror("No message sent.\n");
    }
    return EXIT_SUCCESS;
}

/*
 * This routine handles sigterm signals so that the program can be shutdown gracefully.
 */
void
sigint_handler(int signum)
{
    printf("\tCancelled!\n");
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

/* Run once on startup. */
int
startup(char **argv, int *msg_in, int *msg_out)
{
    key_t msg_in_key, msg_out_key;
    printf("Configuring...");

    /* Setup logging */
    //todo add switch for fileoutput
    g_logfile = stdout;

    /* register interrupt handler */
    if (setup_sigint() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    /* Clear the message queue */
    // TODO


    /* Parse command arguments */
    printf("Starting OMNIUS-CLI in %s...\n", g_bit_mode_str);
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

    /* connect to the incoming msg queue, then outgoing queue - msg_*_key is relative to OMNIUS */
    if (((*msg_in = ipc_connect((key_t) msg_in_key)) < 0) ||
        ((*msg_out = ipc_connect((key_t) msg_out_key)) < 0))
        return EXIT_FAILURE;

    printf("Loaded!\n");
    return EXIT_SUCCESS;
}

/*
 * Close logfile.
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
    fprintf(stderr, "\nERR:%d\nUsage: omnius-cli [msg_in_key] [msg_out_key]\n", ret);
    return;
}

int main(int argc, char ** argv)
{
    int msg_in, msg_out, ret = EXIT_FAILURE;
    key_t msg_in_key, msg_out_key;

    /* Parse cmd-args and setup globals */
    if (argc < 3) {
        /* if these are set, startup will not try and parse cmd args for in/out msgids */
        msg_in = 0xdead1;
        msg_out = 0xdead2;
    }
    printf("Starting OMNIUS command line interface\n");
    if ((ret = startup(argv, &msg_in, &msg_out)) != EXIT_SUCCESS) {
        show_usage(ret);
        exit(ret);
    }
    printf(" Connected... id(in=%d, out=%d)\n", msg_in, msg_out);

    /* start listening for input on stdin */
    ret = cli(msg_in, msg_out);

    printf("Terminating OMNIUS command line interface\n");
    return ret;
}