#ifndef HEADER_fd_src_fdos_host_fdos_cpuid_h
#define HEADER_fd_src_fdos_host_fdos_cpuid_h

#include "../../util/fd_util_base.h"

/* FDOS_CPU_FEAT_* give features that require special CPU configuration
   (e.g. presence of AVX512F implies existence of zmm regs, which
   require XCR0 bits) */

#define FDOS_CPU_FEAT_REG_XMM (1UL<<0)
#define FDOS_CPU_FEAT_REG_YMM (1UL<<1)
#define FDOS_CPU_FEAT_REG_ZMM (1UL<<2)

struct fdos_cpuid_check {
  ulong cpu_feat;

  uint cpuid_01_0 : 1;
  uint cpuid_07_0 : 1;
};

typedef struct fdos_cpuid_check fdos_cpuid_check_t;

void
fdos_cpuid_check_init( fdos_cpuid_check_t * check );

void
fdos_cpuid_check_push( fdos_cpuid_check_t * check,
                       uint                 leaf,
                       uint                 subleaf,
                       uint                 eax,
                       uint                 ebx,
                       uint                 ecx,
                       uint                 edx );

void
fdos_cpuid_validate( fdos_cpuid_check_t const * check );

#endif /* HEADER_fd_src_fdos_host_fdos_cpuid_h */
