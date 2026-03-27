#ifndef HEADER_fd_src_fdos_x86_fd_x86_cpuid_h
#define HEADER_fd_src_fdos_x86_fd_x86_cpuid_h

/* https://www.sandpile.org/x86/cpuid.htm#level_0000_0001h */

#define FD_X86_CPUID_01_ECX_SSE3                (1U<< 0) /* part of x86-64-v2 */
#define FD_X86_CPUID_01_ECX_VMX                 (1U<< 5) /* VMX */
#define FD_X86_CPUID_01_ECX_SSSE3               (1U<< 9) /* part of x86-64-v2 */
#define FD_X86_CPUID_01_ECX_SSE41               (1U<<19) /* part of x86-64-v2 */
#define FD_X86_CPUID_01_ECX_SSE42               (1U<<20) /* part of x86-64-v2 */
#define FD_X86_CPUID_01_ECX_MOVBE               (1U<<22) /* part of x86-64-v3 */
#define FD_X86_CPUID_01_ECX_POPCNT              (1U<<23) /* part of x86-64-v2 */
#define FD_X86_CPUID_01_ECX_AES                 (1U<<25) /* AES-NI */
#define FD_X86_CPUID_01_ECX_AVX                 (1U<<28) /* part of x86-64-v3 */
#define FD_X86_CPUID_01_ECX_HV                  (1U<<31) /* Hypervisor present */

#define FD_X86_CPUID_01_EDX_TSC                 (1U<< 4) /* RDTSC instruction */
#define FD_X86_CPUID_01_EDX_MSR                 (1U<< 5) /* RDMSR/WRMSR instructions */
#define FD_X86_CPUID_01_EDX_PAE                 (1U<< 6) /* 4-level paging */
#define FD_X86_CPUID_01_EDX_PGE                 (1U<<13) /* page table global bit */
#define FD_X86_CPUID_01_EDX_CMOV                (1U<<15) /* cmov instructions */
#define FD_X86_CPUID_01_EDX_CLFL                (1U<<19) /* clflush instruction */
#define FD_X86_CPUID_01_EDX_MMX                 (1U<<23) /* part of x86-64-v1 */
#define FD_X86_CPUID_01_EDX_SSE                 (1U<<25) /* part of x86-64-v1 */
#define FD_X86_CPUID_01_EDX_SSE2                (1U<<26) /* part of x86-64-v1 */

/* https://www.sandpile.org/x86/cpuid.htm#level_0000_0007h */

#define FD_X86_CPUID_07_0_EBX_FSGSBASE          (1U<< 0) /* FSGSBASE */
#define FD_X86_CPUID_07_0_EBX_BMI1              (1U<< 3) /* BMI1, TZCNT */
#define FD_X86_CPUID_07_0_EBX_AVX2              (1U<< 5) /* AVX2, VSIB */
#define FD_X86_CPUID_07_0_EBX_BMI2              (1U<< 8) /* BMI2, MULX, RORX, SARX */
#define FD_X86_CPUID_07_0_EBX_INVPCID           (1U<<10) /* INVPCID */
#define FD_X86_CPUID_07_0_EBX_AVX512F           (1U<<16) /* AVX512F */
#define FD_X86_CPUID_07_0_EBX_AVX512DQ          (1U<<17) /* AVX512DQ */
#define FD_X86_CPUID_07_0_EBX_ADX               (1U<<19) /* ADX */
#define FD_X86_CPUID_07_0_EBX_AVX512IFMA        (1U<<21) /* AVX512IFMA */
#define FD_X86_CPUID_07_0_EBX_PT                (1U<<25) /* Processor Trace */
#define FD_X86_CPUID_07_0_EBX_AVX512PF          (1U<<26) /* AVX512PF */
#define FD_X86_CPUID_07_0_EBX_AVX512ER          (1U<<27) /* AVX512ER */
#define FD_X86_CPUID_07_0_EBX_AVX512CD          (1U<<28) /* AVX512CD */
#define FD_X86_CPUID_07_0_EBX_SHA               (1U<<29) /* SHA extensions */
#define FD_X86_CPUID_07_0_EBX_AVX512BW          (1U<<30) /* AVX512BW */
#define FD_X86_CPUID_07_0_EBX_AVX512VL          (1U<<31) /* AVX512VL */

#define FD_X86_CPUID_07_0_ECX_AVX512VBMI        (1U<< 1) /* AVX512VBMI */
#define FD_X86_CPUID_07_0_ECX_AVX512VBMI2       (1U<< 6) /* AVX512VBMI2 */
#define FD_X86_CPUID_07_0_ECX_CET               (1U<< 7) /* CET */
#define FD_X86_CPUID_07_0_ECX_GFNI              (1U<< 8) /* GFNI */
#define FD_X86_CPUID_07_0_ECX_VAES              (1U<< 9) /* VAES */
#define FD_X86_CPUID_07_0_ECX_VPCL              (1U<<10) /* VPCLMULQDQ */
#define FD_X86_CPUID_07_0_ECX_AVX512VNNI        (1U<<11) /* AVX512VNNI */
#define FD_X86_CPUID_07_0_ECX_AVX512BITALG      (1U<<12) /* AVX512BITALG */
#define FD_X86_CPUID_07_0_ECX_AVX512VPOPCNTDQ   (1U<<14) /* AVX512VPOPCNTDQ */
#define FD_X86_CPUID_07_0_ECX_VA57              (1U<<16) /* 5-level paging */

#define FD_X86_CPUID_07_1_EAX_SHA512            (1U<< 0) /* SHA512 */
#define FD_X86_CPUID_07_1_EAX_AVX_VNNI          (1U<< 4) /* AVX-VNNI */
#define FD_X86_CPUID_07_1_EAX_AVX512BF16        (1U<< 5) /* AVX512BF16 */
#define FD_X86_CPUID_07_1_EAX_FRED              (1U<<17) /* Flexible Return and Event Delivery (FRED) */
#define FD_X86_CPUID_07_1_EAX_LKGS              (1U<<18) /* LKGS */
#define FD_X86_CPUID_07_1_EAX_NMI_SOURCE        (1U<<20) /* FRED NMI SOURCE extension */
#define FD_X86_CPUID_07_1_EAX_AMX_FP16          (1U<<21) /* AMX FP16 */
#define FD_X86_CPUID_07_1_EAX_AVX_IFMA          (1U<<23) /* AVX_IFMA */
#define FD_X86_CPUID_07_1_EAX_LAM               (1U<<26) /* Linear Address Masking */

#define FD_X86_CPUID_07_1_EDX_AVX_VNNI_INT8     (1U<< 4) /* AVX VNNI INT8 */
#define FD_X86_CPUID_07_1_EDX_AMX_COMPLEX       (1U<< 8) /* AMX complex */
#define FD_X86_CPUID_07_1_EDX_AVX_VNNI_INT16    (1U<<10) /* AVX VNNI INT16 */
#define FD_X86_CPUID_07_1_EDX_AVX10             (1U<<19) /* AVX10 */
#define FD_X86_CPUID_07_1_EDX_APX_F             (1U<<21) /* APX Foundation */

/* https://www.sandpile.org/x86/cpuid.htm#level_8000_0001h */

#define FD_X86_CPUID_8000_01_EDX_TSC            (1U<< 4) /* TSC */
#define FD_X86_CPUID_8000_01_EDX_MSR            (1U<< 5) /* MSR */
#define FD_X86_CPUID_8000_01_EDX_PAE            (1U<< 6) /* PAE */
#define FD_X86_CPUID_8000_01_EDX_PGE            (1U<<13) /* PGE */
#define FD_X86_CPUID_8000_01_EDX_CMOV           (1U<<15) /* CMOV */
#define FD_X86_CPUID_8000_01_EDX_NX             (1U<<20) /* NX bit */

#endif /* HEADER_fd_src_fdos_x86_fd_x86_cpuid_h */
