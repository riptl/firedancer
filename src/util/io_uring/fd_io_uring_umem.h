#ifndef HEADER_fd_src_util_io_uring_fd_io_uring_sq_h
#define HEADER_fd_src_util_io_uring_fd_io_uring_sq_h

/* fd_io_uring_sq.h interfaces with io_uring submission queues. */

#include <stdatomic.h>
#include <linux/io_uring.h>
#include "../fd_util_base.h"

/* fd_io_uring_sq_t objects contain pre-allocated submission queue data
   structures. */

struct fd_io_uring_umem {
  atomic_uint * khead;
  atomic_uint * ktail;
  struct io_uring_sqe * sqes;

  uint sqe_head;
  uint sqe_tail;
};

typedef struct fd_io_uring_umem fd_io_uring_umem_t;

FD_PROTOTYPES_BEGIN

ulong
fd_io_uring_sq_align( void );

ulong
fd_io_uring_sq_footprint( ulong depth );

void *
fd_io_uring_sq_new( void * mem,
                    ulong  depth );

fd_io_uring_sq_t *
fd_io_uring_sq_join( fd_io_uring_sq_t * ljoin,
                     void *             mem );

void *
fd_io_uring_sq_leave( fd_io_uring_sq_t * sq );

void *
fd_io_uring_sq_delete( void * mem );

FD_PROTOTYPES_END

#endif /* HEADER_fd_src_util_io_uring_fd_io_uring_sq_h */
