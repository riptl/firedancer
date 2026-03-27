#ifndef HEADER_fd_src_fdos_kern_fdos_kern_def_h
#define HEADER_fd_src_fdos_kern_fdos_kern_def_h

/* Physical memory layout */

#define FDOS_GPADDR_KERN_IMG   0x1000000UL /* kernel image */
#define FDOS_GPADDR_KERN_HEAP  0x2000000UL /* page table, GDT, TSS, etc */
#define FDOS_GPADDR_KERN_STACK 0x3000000UL /* guest stack */
#define FDOS_GPADDR_USER_MEM   0x4000000UL /* user memory */
#define FDOS_GPADDR_SHMEM      0x5000000UL /* external shared memory */

#define FDOS_KERN_STACK_SZ 2*FD_SHMEM_HUGE_PAGE_SZ

/* Virtual memory layout */

#define FDOS_GVADDR_SVM_LO                      0x0UL
#define FDOS_GVADDR_SVM_HI              0x100000000UL
#define FDOS_GVADDR_USER_GVCLOCK     0x7ffffffff000UL
#define FDOS_GVADDR_KERN_HEAP    0xffffc90000000000UL
#define FDOS_GVADDR_KERN_STACK   0xffffff8000000000UL
#define FDOS_GVADDR_KERN_IMG     0xffffffff80000000UL

/* Global Descriptor Table */

#define FDOS_GDT_IDX_NULL      0UL
#define FDOS_GDT_IDX_K32_CS    1UL /* kernel 32-bit code (dummy) */
#define FDOS_GDT_IDX_KERN_CS   2UL /* kernel 64-bit code */
#define FDOS_GDT_IDX_KERN_DS   3UL /* kernel data */
#define FDOS_GDT_IDX_U32_CODE  4UL /* user 32-bit code (dummy), required by FRED */
#define FDOS_GDT_IDX_USER_DS   5UL /* user data */
#define FDOS_GDT_IDX_USER_CS   6UL /* user 64-bit code */
#define FDOS_GDT_IDX_TSS       7UL
#define FDOS_GDT_IDX_TSS_HIGH  8UL
#define FDOS_GDT_CNT           9UL

#endif /* HEADER_fd_src_fdos_kern_fdos_kern_def_h */
