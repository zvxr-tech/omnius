/* omnius-cli/omnius-cli.h
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *

 *
 * 2015 - Mike Clark
 */

#ifndef SECMEM_OMNIUS_CLI_H
#define SECMEM_OMNIUS_CLI_H

#include "../omnius/comm.h"

/* Used to cancel on scanf when null/0 is a valid input and cannot be used to detect a user's intention to cancel when
 * awaiting user input
 */
#define ESC_CHAR 0x1b


int scan_mtype_load(blob_t *);
int scan_mtype_unload(blob_t *);
int scan_mtype_alloc(blob_t *);
int scan_mtype_dealloc(blob_t *);
int scan_mtype_read(blob_t *);
int scan_mtype_write(blob_t *);
int scan_mtype_read(blob_t *);
int scan_mtype_view(blob_t *);
int scan_mtype_test(blob_t *);

void print_view(msgbuf_t);
int cli(int, int);

void sigint_handler(int);
int setup_sigint(void);

int startup(char **, int *, int *);
void shutdown(void);

void show_usage(int ret);
int main(int, char **);

#endif //SECMEM_OMNIUS_CLI_H
