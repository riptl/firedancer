#ifndef HEADER_fd_src_fdos_fdos_pvclock_h
#define HEADER_fd_src_fdos_fdos_pvclock_h

#include "../util/bits/fd_bits.h"

struct __attribute__((packed)) fd_pvclock {
  struct __attribute__((packed)) {
    uint version;
    uint sec;
    uint nsec;
  } base;
  struct __attribute__((packed)) {
    uint  version;
    uint  pad0;
    ulong tsc_timestamp;
    ulong system_time;
    uint  tsc_to_system_mul;
    schar tsc_shift;
    uchar flags;
    uchar pad[2];
  } off;
};

typedef struct fd_pvclock fd_pvclock_t;

static inline ulong 
pvclock_scale_delta( ulong delta, 
                     uint  mul_frac,
                     int   shift) {
	if( shift < 0 ) delta >>= -shift;
	else            delta <<= shift;

	ulong product;
	ulong tmp;
	__asm__ (
		"mulq %[mul_frac];\n"
    "shrd $32, %[hi], %[lo]\n"
		: [lo] "=a" (product),
		  [hi] "=d" (tmp)
		:            "0"  (delta),
		  [mul_frac] "rm" ((ulong)mul_frac)
  );

	return product;
}

FD_FN_UNUSED __attribute__((noinline)) static long
fd_pvclock_now( void const * c ) {
  /* FIXME version checking */
  fd_pvclock_t const * pvclock = (fd_pvclock_t const *)c;
  ulong t = __builtin_ia32_rdtsc() - pvclock->off.tsc_timestamp;
  t  = pvclock_scale_delta( t, pvclock->off.tsc_to_system_mul, pvclock->off.tsc_shift );
  t += pvclock->off.system_time;
  t += ((ulong)pvclock->base.sec)*((ulong)1e9) + (ulong)pvclock->base.nsec;
  return (long)t;
}

#endif /* HEADER_fd_src_fdos_fdos_pvclock_h */
