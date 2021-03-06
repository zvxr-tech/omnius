/* omnius/comm.c
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 *
 * These are helper routines associated with omnius' communications.
 *
 * 2015 - Mike Clark
 */
#include <stdio.h>
#include "comm.h"

/*
 * Make a human-readable string describing a blob.
 */
size_t
humanize_blob(msgbuf_t *msg_buf, char *out, size_t out_size)
{
    size_t len = 0;
    if (IS_MTYPE_REPLY(msg_buf->mtype)) {

        /* REPLIES - strip off the ack/nak flag */
        switch (STRIP_MTYPE_MOD(msg_buf->mtype)) {
            case MTYPE_LOAD:
                len = (size_t) snprintf(out, out_size, "Loaded pid %d\n", msg_buf->blob.head.pid);
                break;
            case MTYPE_UNLOAD:
                len = (size_t) snprintf(out, out_size, "Unloaded pid %d\n", msg_buf->blob.head.pid);
                break;
            case MTYPE_ALLOC:
                /* the semantics of this message change if memory allocation failed */
		len = (size_t) snprintf(out, out_size, "Allocated from pid %d @ secmem %s 0x%lx\n", msg_buf->blob.head.pid,
                                         IS_ACK(msg_buf->mtype) ? "address" : "size",
                                         IS_ACK(msg_buf->mtype) ? msg_buf->blob.head.addr : msg_buf->blob.head.size);
                break;
            case MTYPE_DEALLOC:
                len = (size_t) snprintf(out, out_size, "Deallocated from pid %d\n", msg_buf->blob.head.pid);
                break;
            case MTYPE_READ:
                len = (size_t) snprintf(out, out_size, "Read from pid %d\n", msg_buf->blob.head.pid);
                break;
            case MTYPE_WRITE:
                len = (size_t) snprintf(out, out_size, "Written to pid %d\n", msg_buf->blob.head.pid);
                break;
            case MTYPE_VIEW:
                len = (size_t) snprintf(out, out_size, "Viewed message pid %d\n", msg_buf->blob.head.pid);
                break;
            case MTYPE_TERMINATE:
                len = (size_t) snprintf(out, out_size, "Terminated message\n");
                break;
            case MTYPE_NIL:
                len = (size_t) snprintf(out, out_size, "NIL message\n");
                break;
            default:
                len = (size_t) snprintf(out, out_size, "UNKNOWN message type (%d) ", (int) msg_buf->mtype);
                break;
        }
        len += snprintf(out + len, out_size - len, (IS_ACK(msg_buf->mtype) ? "ACK\n" : "NAK\n"));
    } else {
        /* REQUESTS */
        switch (msg_buf->mtype) {
            case MTYPE_LOAD:
                len = (size_t) snprintf(out, out_size, "Loading %zu policies for pid %d with %zu bytes of secmem memory in total.\n",
                                               (size_t) msg_buf->blob.head.policy_count, msg_buf->blob.head.pid, (size_t) msg_buf->blob.head.size);
                break;
            case MTYPE_UNLOAD:
                len = (size_t) snprintf(out, out_size, "Unloading pid %d.\n", msg_buf->blob.head.pid);
                break;
            case MTYPE_ALLOC:
                len = (size_t) snprintf(out, out_size, "Allocating %lx bytes from pid %d, size 0x%lx\n", msg_buf->blob.head.size, msg_buf->blob.head.pid,
                               (size_t) msg_buf->blob.head.size);
                break;
            case MTYPE_DEALLOC:
                len = (size_t) snprintf(out, out_size, "Deallocating from pid %d @ secmem address 0x%lx\n", msg_buf->blob.head.pid,
                               (size_t) msg_buf->blob.head.addr);
                break;
            case MTYPE_READ:
                len = (size_t) snprintf(out, out_size, "Reading %lx bytes from pid %d @ secmem addres 0x%lx\n", msg_buf->blob.head.data_len, msg_buf->blob.head.pid,
                               (size_t) msg_buf->blob.head.addr);
                break;
            case MTYPE_WRITE:
                len = (size_t) snprintf(out, out_size, "Writing %lx bytes to pid %d @ secmem addres 0x%lx\n", msg_buf->blob.head.data_len, msg_buf->blob.head.pid,
                               (size_t) msg_buf->blob.head.addr);
                break;
            case MTYPE_VIEW:
                len = (size_t) snprintf(out, out_size, "VIEW message.\n");
                break;
            case MTYPE_TERMINATE:
                len = (size_t) snprintf(out, out_size, "TERMINATION message.\n");
                break;
            case MTYPE_NIL:
                len = (size_t) snprintf(out, out_size, "NIL message.\n");
                break;
            default:
                len = (size_t) snprintf(out, out_size, "UNKNOWN message type (%d).\n", (int) msg_buf->mtype);
                break;
        }
    }
    
    len += snprintf(out + len, out_size - len,"Data:\n");
    if (IS_MTYPE_VIEW_ACK(msg_buf->mtype)) {
        len = len; /* TODO finish implementing this: print_view(out_buf); */
    } else {
        char eol = '\t';
        size_t i;
        for (i = 0;  len <= out_size && i < msg_buf->blob.head.data_len; i++) {
            unsigned int  holder = (unsigned int) msg_buf->blob.body.data[i];
            len += snprintf(out + len, out_size - len,"%x%c", holder, eol);
            eol = (char) ((((i + 1) % VIEW_DATA_ROW_WIDTH) == 0) ? '\n' : '\t');
        }
    }
    return len;
}

/*
 * Return -1 on error, otherwise the message queue id is returned.
 */
int
ipc_connect(key_t key)
{
    printf("\tConnecting to message queue 0x%08x ...", key);

    int msgqid;
    if ((msgqid = msgget(key, 0644 | IPC_CREAT)) == -1)
    {
        perror("msgget");
        return -1;
    }
    printf("Connected!\n");
    return msgqid;
}

/*
 * Does nothing. Always returns zero.
 */
int
ipc_disconnect(void)
{
    return 0; /*  no need to do anything. */
}
