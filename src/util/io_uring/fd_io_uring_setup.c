#include "fd_io_uring_setup.h"
#include "../shmem/fd_shmem.h"

ulong
fd_io_uring_shmem_align( void ) {
  return FD_SHMEM_NORMAL_PAGE_SZ;
}

static ulong
fd_io_uring_shmem_layout( ulong   sq_depth,
                          ulong   cq_depth,
                          ulong * sqe_off ) {
  if( FD_UNLIKELY( !fd_ulong_is_pow2( sq_depth ) ) ) return 0UL;
  if( FD_UNLIKELY( !fd_ulong_is_pow2( cq_depth ) ) ) return 0UL;

  ulong cq_sz;
  if( FD_UNLIKELY( __builtin_umull_overflow( cq_depth, sizeof(struct io_uring_cqe), &cq_sz ) ) ) return 0UL;
  ulong sqa_sz;
  if( FD_UNLIKELY( __builtin_umull_overflow( sq_depth, sizeof(uint), &sqa_sz ) ) ) return 0UL;

  /* io_uring CQ region

     This API matches Linux io_uring.c rings_size():
     https://elixir.bootlin.com/linux/v6.11.5/source/io_uring/io_uring.c#L2559 */

  FD_SCRATCH_ALLOC_INIT( l, NULL );

  /* The true footprint requirement depends on the kernel version.  The
     head part of this region is 'struct io_rings', which is not stable
     ABI.  We use a very conservative 4 KiB here. */

  FD_SCRATCH_ALLOC_APPEND( l, FD_SHMEM_NORMAL_PAGE_SZ, 4096UL );

  /* Completion queue (cache line align) */

  FD_SCRATCH_ALLOC_APPEND( l, 128UL, cq_depth*sizeof(struct io_uring_cqe) );

  /* Submission queue index array (cache line align) */

  FD_SCRATCH_ALLOC_APPEND( l, 128UL, sq_depth*sizeof(uint) );

  /* io_uring SQEs region */

  *sqe_off = (ulong)FD_SCRATCH_ALLOC_APPEND(
      l, FD_SHMEM_NORMAL_PAGE_SZ, sq_depth*sizeof(struct io_uring_sqe) );

  return FD_SCRATCH_ALLOC_FINI( l, FD_SHMEM_NORMAL_PAGE_SZ );
}

ulong
fd_io_uring_shmem_footprint( ulong   sq_depth,
                             ulong   cq_depth ) {
  ulong sqe_off_;
  return fd_io_uring_shmem_layout( sq_depth, cq_depth, &sqe_off_ );
}

struct io_uring_params *
fd_io_uring_shmem_setup( struct io_uring_params * params,
                         void *                   shmem,
                         ulong                    sq_depth,
                         ulong                    cq_depth ) {

  ulong sqe_off;
  ulong shmem_footprint = fd_io_uring_shmem_layout( sq_depth, cq_depth, &sqe_off );
  if( FD_UNLIKELY( !shmem_footprint ) ) {
    FD_LOG_WARNING(( "invalid sq_depth (%lu) or cq_depth (%lu)", sq_depth, cq_depth ));
    return NULL;
  }

  params->flags |= IORING_SETUP_NO_MMAP;
  params->sq_entries = (uint)sq_depth;
  params->cq_entries = (uint)cq_depth;

  /* cq_off points to the region containing the kernel private io_rings
     struct, the completion queue (array of CQEs), and the submission
     queue array (array of uints). */

  params->cq_off = (struct io_cqring_offsets) {
    .user_addr = (unsigned long long)( (uchar *)shmem ),
  };

  /* sq_off points to the table of submission queue entries. */

  params->sq_off = (struct io_sqring_offsets) {
    .user_addr = (unsigned long long)( (uchar *)shmem + sqe_off ),
  };

  return params;
}
