/* omnius/omnius.h
 * Copyright 2015 Mike Clark
 * Distributed under the GNU General Public License V.2
 *
 *
 * 2015 - Mike Clark
 */

#ifndef SECMEM_OMNIUS_H
#define SECMEM_OMNIUS_H

#include "comm.h"

#define OMNIUS_RET_SUCCESS 0
#define OMNIUS_RET_ARGS 1
#define OMNIUS_RET_CONFIG 2
#define OMNIUS_RET_MSG 3
#define OMNIUS_RET_DISPATCH 4
#define OMNIUS_RET_SIGNAL 5
#define OMNIUS_RET_COUNT 6

#define MAX_PID 32768


int
omnius_load(blob_t *);

int
omnius_unload(blob_t *);

int
omnius_alloc(blob_t *);

int
omnius_dealloc(blob_t *);

int
omnius_read(blob_t *);

int
omnius_write(blob_t *);

int
omnius_view(blob_t *);

int
omnius_view_internal(blob_t *);

int
omnius_nil(blob_t *);

int
listen(int, int);

void
sigint_handler(int);

int
setup_sigint(void);

int
startup(char **, int *, int *);

void
shutdown(void);

void
show_usage(int ret);

int
main(int, char **);


#endif /* SECMEM_OMNIUS_H */
