#ifndef HEADER_fd_src_util_io_uring_fd_io_uring_h
#define HEADER_fd_src_util_io_uring_fd_io_uring_h

/* fd_io_uring.h provides APIs for job submission and completion polling
   against io_uring instances.

   These APIs are suitable for cooperative or interrupt-driven I/O only
   (completions delivered on the thread that submitted requests). These
   APIs do not support busy polling with kernel worker threads  */

#ifndef _GNU_SOURCE
#error "fd_io_uring_register.h requires _GNU_SOURCE"
#endif
#include "../fd_util_base.h"
#include <stdatomic.h>
#include <sys/syscall.h>
#include <linux/io_uring.h>

struct fd_io_uring_sq {

  /* State bits shared with the kernel.

     The kernel might set these in an interrupt context, therefore we
     accesses to be explicit.  We assume no concurrent operation
     (although io_uring supports such operation), so accesses to these
     do not need stronger consistency than C11 relaxed. */

  atomic_uint * khead;
  atomic_uint * ktail;
  atomic_uint * kflags;
  atomic_uint * kdropped;

  uint *                array;
  struct io_uring_sqe * sqes;

  uint sqe_head;
  uint sqe_tail;
  uint depth;
};

typedef struct fd_io_uring_sq fd_io_uring_sq_t;

struct fd_io_uring_cq {
  ulong depth;

  atomic_uint * khead;
  atomic_uint * ktail;
  atomic_uint * koverflow;

  struct io_uring_cqe * cqes;
};

typedef struct fd_io_uring_cq fd_io_uring_cq_t;

struct fd_io_uring {
  int ioring_fd;

  fd_io_uring_sq_t sq[1];
  fd_io_uring_cq_t cq[1];
};

typedef struct fd_io_uring fd_io_uring_t;

FD_PROTOTYPES_BEGIN

/* io_uring_enter syscall wrapper */

static inline int
fd_io_uring_enter( int    ring_fd,
                   uint   to_submit,
                   uint   min_complete,
                   uint   flags,
                   void * arg,
                   ulong  arg_sz ) {
  return (int)syscall( SYS_io_uring_enter, ring_fd, to_submit, min_complete, flags, arg, arg_sz );
}

/* fd_io_uring_submit flushes the submission queue, enters io_uring with
   GETEVENTS, and waits for wait_cnt completions to arrive.  Returns 0
   on success, -1 on error (sets errno). */

FD_FN_UNUSED static int
fd_io_uring_submit( fd_io_uring_sq_t * sq,
                    int                ring_fd,
                    uint               wait_cnt,
                    uint               flags ) {
  uint tail = sq->sqe_tail;
  atomic_store_explicit( sq->ktail, tail, memory_order_release );
  uint to_submit = tail - atomic_load_explicit( sq->khead, memory_order_relaxed );
  return fd_io_uring_enter( ring_fd, to_submit, wait_cnt, flags, NULL, 0 );
}

static inline struct io_uring_sqe *
fd_io_uring_get_sqe( fd_io_uring_sq_t * sq ) {
  uint tail  = sq->sqe_tail;
  uint depth = sq->depth;
  if( tail+1U - sq->sqe_head > depth ) {
    return NULL;
  }
  sq->sqe_tail = tail+1U;
  return &sq->sqes[ tail & (depth-1U) ];
}

/* fd_io_uring_sq_space_left returns the lower bound on the number of
   free SQEs. */

static inline uint
fd_io_uring_sq_space_left( fd_io_uring_sq_t const * sq ) {
  uint pending = sq->sqe_tail - atomic_load_explicit( sq->khead, memory_order_relaxed );
  return (uint)sq->depth - pending;
}

/* fd_io_uring_cq_ready returns the lower bound on the number of CQEs
   not yet received. */

static inline uint
fd_io_uring_cq_ready( fd_io_uring_cq_t const * cq ) {
  uint tail = atomic_load_explicit( cq->ktail, memory_order_acquire );
  uint head = atomic_load_explicit( cq->khead, memory_order_relaxed );
  return tail - head;
}

static inline void
fd_io_uring_cq_advance( fd_io_uring_cq_t * cq,
                        uint               cnt ) {
  uint head = atomic_load_explicit( cq->khead, memory_order_relaxed );
  atomic_store_explicit( cq->khead, head + cnt, memory_order_release );
}

static inline struct io_uring_cqe *
fd_io_uring_cq_head( fd_io_uring_cq_t const * cq ) {
  uint head = atomic_load_explicit( cq->khead, memory_order_relaxed );
  return &cq->cqes[ head & (cq->depth - 1U) ];
}

FD_PROTOTYPES_END

#endif /* HEADER_fd_src_util_io_uring_fd_io_uring_h */
