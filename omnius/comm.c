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
#include <sys/msg.h>
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
                len = (size_t) snprintf(out, out_size, "\nLoading pid %d", msg_buf->blob.head.pid);
                break;
            case MTYPE_UNLOAD:
                len = (size_t) snprintf(out, out_size, "\nUnloading pid %d", msg_buf->blob.head.pid);
                break;
            case MTYPE_ALLOC:
                len = (size_t) snprintf(out, out_size, "\nAllocating from pid %d address %x", msg_buf->blob.head.pid,
                                         msg_buf->blob.head.addr);
                break;
            case MTYPE_DEALLOC:
                len = (size_t) snprintf(out, out_size, "\nDeallocating from pid %d", msg_buf->blob.head.pid);
                break;
            case MTYPE_READ:
                len = (size_t) snprintf(out, out_size, "\nReading from pid %d", msg_buf->blob.head.pid);
                break;
            case MTYPE_WRITE:
                len = (size_t) snprintf(out, out_size, "\nWriting to pid %d", msg_buf->blob.head.pid);
                break;
            case MTYPE_VIEW:
                len = (size_t) snprintf(out, out_size, "\nVIEW message pid %d", msg_buf->blob.head.pid);
                break;
            case MTYPE_TERMINATE:
                len = (size_t) snprintf(out, out_size, "\nTERMINATION message ");
                break;
            case MTYPE_NIL:
                len = (size_t) snprintf(out, out_size, "\nNIL message ");
                break;
            default:
                len = (size_t) snprintf(out, out_size, "\nUNKNOWN message type (%d) ", (int) msg_buf->mtype);
                break;
        }
        snprintf(out + len, out_size - len, IS_ACK(msg_buf->mtype) ? "\tACK\n" : "\tNAK\n");
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
                len = (size_t) snprintf(out, out_size, "Allocating from pid %d, size %zu\n", msg_buf->blob.head.pid,
                               (size_t) msg_buf->blob.head.size);
                break;
            case MTYPE_DEALLOC:
                len = (size_t) snprintf(out, out_size, "Deallocating from pid %d @ local offset %zu\n", msg_buf->blob.head.pid,
                               (size_t) msg_buf->blob.head.addr);
                break;
            case MTYPE_READ:
                len = (size_t) snprintf(out, out_size, "Reading from pid %d @ local offset %zu\n", msg_buf->blob.head.pid,
                               (size_t) msg_buf->blob.head.addr);
                break;
            case MTYPE_WRITE:
                len = (size_t) snprintf(out, out_size, "Writing to pid %d @ local offset %zu\n", msg_buf->blob.head.pid,
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
    printf("DATA:\n");
    if (IS_MTYPE_VIEW_ACK(msg_buf->mtype)) {
        len = len; /* TODO finish implementing this: print_view(out_buf); */
    } else {
        char eol = '\t';
        size_t i;
        for (i = 0;  len <= out_size && i < msg_buf->blob.head.data_len; i++) {
            len += snprintf(out + len, out_size - len,"%x%c", msg_buf->blob.body.data[i], eol);
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