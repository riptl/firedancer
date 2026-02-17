#ifndef HEADER_fd_src_fdos_x86_fd_x86_mmu_h
#define HEADER_fd_src_fdos_x86_fd_x86_mmu_h

#include "../../util/fd_util_base.h"

#define FD_X86_PML4E_RANGE (1UL<<39)
#define FD_X86_PML3E_RANGE (1UL<<30)
#define FD_X86_PML2E_RANGE (1UL<<21)
#define FD_X86_PML1E_RANGE (1UL<<12)

#define FD_X86_CR3_LAM57    (1UL<<61)
#define FD_X86_CR3_LAM48    (1UL<<62)
#define FD_X86_CR3_KEEP_TLB (1UL<<63) /* don't invalidate TLB */

#define FD_X86_PT_P  (1UL<< 0) /* present */
#define FD_X86_PT_RW (1UL<< 1) /* read-write */
#define FD_X86_PT_US (1UL<< 2) /* user */
#define FD_X86_PT_A  (1UL<< 5) /* accessed */
#define FD_X86_PT_D  (1UL<< 6) /* dirty */
#define FD_X86_PT_PS (1UL<< 7) /* page size (0=directory 1=page) */
#define FD_X86_PT_G  (1UL<< 8) /* global */
#define FD_X86_PT_XD (1UL<<63) /* execute disable */

#define FD_X86_PM_SZ FD_X86_PML1E_RANGE

static inline ulong
fd_x86_mmu_paddr( ulong pte ) {
  return pte & 0x000ffffffffff000UL;
}

#endif /* HEADER_fd_src_fdos_x86_fd_x86_mmu_h */
