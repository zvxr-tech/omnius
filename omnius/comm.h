/* omnius/comm.h
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 *
 *
 * This header can be used in other programs so that they can communicate with omnius.
 *
 * 2015 - Mike Clark
 */
#ifndef SECMEM_COMM_H
#define SECMEM_COMM_H

#include <signal.h>
#include <sys/types.h>
#include "global.h"

/*
 * These constants are used to tag a message. Reply messages are OR'd with an ACK or NAK depending on the success of the
 * action triggered within omnius.
 */

/* MTYPE bases */
#define MTYPE_NIL 	    0x00
#define MTYPE_LOAD 	    0x01
#define MTYPE_UNLOAD 	0x02
#define MTYPE_ALLOC 	0x03
#define MTYPE_DEALLOC 	0x04
#define MTYPE_READ 	    0x05
#define MTYPE_WRITE 	0x06
#define MTYPE_VIEW 	    0x07
#define MTYPE_TERMINATE 0x08
#define MTYPE_COUNT 	0x09

/*MTYPE modifiers */
#define MTYPE_MOD_ACK 	0x10
#define MTYPE_MOD_NAK 	0x20

/*
 * Helper macros, with self-explanatory identifiers.
 */
#define STRIP_MTYPE_MOD(_mtype) ((_mtype) & (~(MTYPE_MOD_ACK | MTYPE_MOD_NAK)))
#define IS_MTYPE_REPLY(_mtype) ((_mtype) & (MTYPE_MOD_ACK | MTYPE_MOD_NAK))
#define IS_MTYPE_VIEW_ACK(_mtype) ((_mtype)  == (MTYPE_VIEW && (_mtype) & MTYPE_MOD_ACK))
#define IS_ACK(_mtype) ((_mtype) & MTYPE_MOD_ACK)
#define IS_NAK(_mtype) ((_mtype) & MTYPE_MOD_NAK)



/*
 * These define constants associated with the IPC message structure used to wrap all communications into omnius, as well
 * as the structs embedded within.
 *
 * Read/Write operations are the only routines that  have a chance of filling the blob data section. The
 * MAX_BLOB_DATA_SIZE macro effectively becomes the limitation on RW data transfers.
 * The 'LG' size is used by the clear-message queue program so that it can grab erronously sized packets larger than
 * the default
 */
#define MAX_BLOB_DATA_SIZE 4096
#define MAX_MTEXT_SIZE  (sizeof(blob_header_t) + MAX_BLOB_DATA_SIZE)
#define MAX_MTEXT_LG_SIZE (sizeof(blob_header_t) + MAX_BLOB_DATA_SIZE)
#define SIZEOF_BLOB(_pobj) (sizeof(blob_header_t) + (_pobj)->head.data_len)
#define SIZEOF_POLICY(_pobj) (sizeof(policy_head_t) + (_pobj)->head.len)
#define NEXT_POLICY(_pobj) ((policy_t *)((char *)(_pobj) + sizeof(policy_head_t) + (_pobj)->head.len))

/* These are the input symbols used in a policies RW access regex. They are used to map the symbols in the regex to
 * an internal enumeration used by the FSM.
 */
#define READ_CHAR 'R'
#define WRITE_CHAR 'W'

/* This is used to determine the buffer size to hold human readable text describing a message */
#define MAX_HUMANIZE_LEN 255
#define VIEW_DATA_ROW_WIDTH 16

/*
 * POLICY STUCTURES
 * This is a raw description of a policy.
 * It includes:
 *  -the count of symbols the regex uses (from the alphabet [0-255]). This includes the NULL symbol, which may not be
 *   explicitly used in the regex input during policy loading.
 *  -the length of the regular expression, and
 *  -the regular expression (ASCII encoded char sequence and unterminated).
 *      The regex is a format that can be understood by the ragasm compiler.
 *      i.e. "RWR+(WW)*R"
 *
 * The fields are grouped into a head (symbol count, regex len) and a body (regex) using sub-typing to make programatic
 * usage easier.
 *
 */
typedef struct policy_body_t
{
    char regex[1];
} policy_body_t;

typedef struct policy_head_t
{
    SECMEM_INTERNAL_T symbol_count;
    SECMEM_INTERNAL_T len;
} policy_head_t;

typedef struct policy_t
{
    policy_head_t head;
    policy_body_t body;
} policy_t;

/*`
 * BLOB STUCTURES
 *
 * A blob is a container to hold message data and metadata delivered to omnius.
 * It is composed of a header and data section. The data can either be opaque data (cast as an array of type char), or
 * it can be treated as an array of policy_entries, accessing via two union alias'. Either sub-type is of the same size
 * MAX_BLOB_DATA_SIZE, which is calculated by subtracting the blob header size (fixed) from the maximum size of the
 * MTEXT msgbuf_t field. (since the blob is the mtext section).
 *
 * The header uses unions to partition the struct into 4 fields. Each field has multiple aliases that can be used
 * depending on the type of messsage determined by msgbuf_t's MTYPE field. All members are of identical size to the
 * other members of that union (field), this minimizes the chance of errors parsing the message, however this is simply
 * an ad-hoc, POCish way to communicate with no regard to the safety or welfare of others. BE WARNED!
 *
 */

typedef struct blob_header_t {
    /* Field 1 */
    union {
        pid_t field1;
        pid_t pid; /* ALL */
    };

    /* Field 2 */
    union {
        SECMEM_INTERNAL_T field2;
        SECMEM_INTERNAL_T size;     /* load, alloc */
        SECMEM_INTERNAL_T addr;     /* dealloc, read, write */
    };

    /* Field 3 */
    union {
        SECMEM_INTERNAL_T field3;
        SECMEM_INTERNAL_T policy_count; /* load */
        SECMEM_INTERNAL_T policy_id;    /* alloc */
    };

    /* Field 4 */
    union {
        SECMEM_INTERNAL_T field4;
        SECMEM_INTERNAL_T data_len; /* ALL */
    };
} blob_header_t;

typedef struct blob_data_t {
    union {
        char data[MAX_BLOB_DATA_SIZE];
        policy_t policy_entry[1];
    };
} blob_data_t;

typedef struct blob_t
{
        blob_header_t head;
        blob_data_t body;
} blob_t;



/*
 * MSGBUF STUCTURES
 *
 * This data type is used by IPC message queue's to transport the messages.
 * We use the MTYPE field to tag a message by the action we wish to induce in omnius (load, unload, alloc, dealloc,
 * read, write, terminate, view).
 * The second fieldfield can either be treated as an opaque array of type char, or it can be
 * interpreted as type blob_t. Both interpretations are of the same fixed maximum size (MAX_MTEXT_SIZE) so they can be
 * implemented using a union.
 *
 * A second large msgbuf_LG_t is used by a message buffer clearing utility. It has a larger mtext buffer so that it can clear
 * (possibly) errornous IPC messages that are too large for the regular msgbuf_t to hold. This type should NOT be used
 * in the regular omnius configuration. Or you could just send it an msgctl.
 *
 */

typedef struct msgbuf_LG_t {
    long mtype;
    union {
        char mtext[MAX_MTEXT_LG_SIZE];
        blob_t blob;
    };
} msgbuf_LG_t;

typedef struct msgbuf_t
{
    long mtype;
    union {
        char mtext[MAX_MTEXT_SIZE];
        blob_t blob;
    };
} msgbuf_t;


/*
 * HUMANIZE_BLOB ROUTINE
 *
 * This routine will take a message and generate a null-terminated, human readable string describing it.
 * This expects a caller allocated buffer to be passed in as the second argument, and a maximium buffer length as
 * the third.
 */
size_t
humanize_blob(msgbuf_t *, char *, size_t);


/*
 * IPC_CONNECT, IPC_DISCONNECT ROUTINES
 *
 * The connect routine is used to setup a connection to an IPC message queue, it takes in a key_t which is simply an int
 * value.
 *
 * The disconnect is not needed and is merely a stub.
 *
 */

int
ipc_connect(key_t);

int
ipc_disconnect(void);

#endif /* SECMEM_COMM_H */
