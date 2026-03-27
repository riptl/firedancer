#ifndef HEADER_fd_src_fdos_x86_fd_x86_msr_h
#define HEADER_fd_src_fdos_x86_fd_x86_msr_h

#define FD_X86_RFLAGS_IDX_PF    2
#define FD_X86_RFLAGS_IDX_TF    8
#define FD_X86_RFLAGS_IDX_IOPL 12

#define FD_X86_RFLAGS_PF    (1UL<<FD_X86_RFLAGS_IDX_PF)
#define FD_X86_RFLAGS_TF    (1UL<<FD_X86_RFLAGS_IDX_TF)
#define FD_X86_RFLAGS_IOPL3 (3UL<<FD_X86_RFLAGS_IDX_IOPL)

#define FD_X86_CR0_PE (1U<<0)
#define FD_X86_CR0_MP (1U<<1)
#define FD_X86_CR0_EM (1U<<2)
#define FD_X86_CR0_TS (1U<<3)
#define FD_X86_CR0_ET (1U<<4)
#define FD_X86_CR0_NE (1U<<5)
#define FD_X86_CR0_WP (1U<<16)
#define FD_X86_CR0_AM (1U<<18)
#define FD_X86_CR0_NW (1U<<29)
#define FD_X86_CR0_CD (1U<<30)
#define FD_X86_CR0_PG (1U<<31)

#define FD_X86_CR4_PAE      (1UL<< 5)
#define FD_X86_CR4_PGE      (1UL<< 7)
#define FD_X86_CR4_OSFXSR   (1UL<< 9)
#define FD_X86_CR4_FSGSBASE (1UL<<16)
#define FD_X86_CR4_OSXSAVE  (1UL<<18)
#define FD_X86_CR4_FRED     (1UL<<32)

#define FD_X86_EFER_SCE (1U<< 0)
#define FD_X86_EFER_LME (1U<< 8)
#define FD_X86_EFER_LMA (1U<<10)
#define FD_X86_EFER_NXE (1U<<11)

#define FD_X86_MSR_FRED_RSP0    0x000001cc
#define FD_X86_MSR_FRED_RSP1    0x000001cd
#define FD_X86_MSR_FRED_RSP2    0x000001ce
#define FD_X86_MSR_FRED_RSP3    0x000001cf
#define FD_X86_MSR_FRED_STKLVLS 0x000001d0
#define FD_X86_MSR_FRED_SSP1    0x000001d1
#define FD_X86_MSR_FRED_SSP2    0x000001d2
#define FD_X86_MSR_FRED_SSP3    0x000001d3
#define FD_X86_MSR_FRED_CONFIG  0x000001d4

#define FD_X86_MSR_EFER          0xc0000080
#define FD_X86_MSR_STAR          0xc0000081
#define FD_X86_MSR_LSTAR         0xc0000082
#define FD_X86_MSR_FSBASE        0xc0000100
#define FD_X86_MSR_PVCLOCK_EPOCH 0x4b564d00
#define FD_X86_MSR_PVCLOCK_OFF   0x4b564d01

#define FD_X86_XCR0_X87       (1U<<0)
#define FD_X86_XCR0_SSE       (1U<<1)
#define FD_X86_XCR0_AVX       (1U<<2)
#define FD_X86_XCR0_OPMASK    (1U<<5)
#define FD_X86_XCR0_ZMM_HI256 (1U<<6)
#define FD_X86_XCR0_HI16_ZMM  (1U<<7)

#endif /* HEADER_fd_src_fdos_x86_fd_x86_msr_h */
