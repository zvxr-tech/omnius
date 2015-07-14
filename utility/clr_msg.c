/* utility/clr_msg.c
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *

 * Utility to flush a message buffer queue without killing it.
 *
 * 2015 - Mike Clark
 */
#include <sys/types.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <stdio.h>
#include "../omnius/comm.h"

int main(int argc, char **argv)
{
    key_t key;

    if (argc > 1) {
        key = (key_t) strtol(argv[1], NULL, 16);
    } else {
        printf("ERR: No key provided.\nUsage: msg_clr <msg_key>\n");
        return 1;
    }

    printf("Are you sure you want to clear the contents of message queue key=%x.\t(enter any number to continue) ?\n", key);
    int d = 0;
    while (d == 0) {
        scanf("%d", &d);
    }

    int msgqid = msgget(key, 0644 | IPC_CREAT);
    if (msgqid !=  -1)
    {
        printf("SUCCESS: omnius_out-connect\nkey=%x\nmsgid=%d\n", (int) key, msgqid);
        struct msgbuf_LG_t buf;
        size_t len = sizeof(buf.mtext);
        while (msgrcv(msgqid, &buf, len, 0, 0) != -1) {printf("RECV\n"); }
    }
    else
    {
        printf("ERROR: omnius_out-connect\n");
    }

    return 0;
}