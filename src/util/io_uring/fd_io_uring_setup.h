#ifndef HEADER_fd_src_util_io_fd_io_uring_setup_h
#define HEADER_fd_src_util_io_fd_io_uring_setup_h

/* fd_io_uring_setup.h provides an API to setup Linux io_uring
   instances. */

#ifndef _GNU_SOURCE
#error "fd_io_uring_register.h requires _GNU_SOURCE"
#endif
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/io_uring.h>
#include "../../util/fd_util_base.h"

FD_PROTOTYPES_BEGIN

/* IORING_SETUP_NO_MMAP related ***************************************/

/* fd_io_uring_shmem_{align,footprint} return the required alignment
   and footprint for a user-managed shared memory region suitable to
   hold io_uring data structures.  This includes the submission queue
   array, the submission queue entries, and the completion queue.
   {sq,cq}_depth must be powers of two.  footprint returns non-zero on
   success, and 0 (silently) if {sq,cq}_depth are invalid. */

ulong
fd_io_uring_shmem_align( void );

ulong
fd_io_uring_shmem_footprint( ulong sq_depth,
                             ulong cq_depth );

/* fd_io_uring_shmem_setup adds a user-managed shared memory region to
   params.  params is zero initialized by the caller.  shmem points to
   a region allocated according to the above align/footprint
   requirements.  Sets the IORING_SETUP_NO_MMAP flag, which instructs
   the kernel to map user memory instead of allocating new rings.

   Returns params on success.  On failure, returns NULL.  Reasons for
   failure include obviously invalid shmem pointer or invalid
   {sq,cq}_depth.  Logs reason for failure to WARNING. */

struct io_uring_params *
fd_io_uring_shmem_setup( struct io_uring_params * params,
                         void *                   shmem,
                         ulong                    sq_depth,
                         ulong                    cq_depth );

/* Setup API **********************************************************/

static inline int
fd_io_uring_setup( uint                     entry_cnt,
                   struct io_uring_params * p ) {
  return (int)syscall( SYS_io_uring_setup, entry_cnt, p );
}

FD_PROTOTYPES_END

#endif /* HEADER_fd_src_util_io_fd_io_uring_setup_h */
