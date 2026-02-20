#ifndef HEADER_fd_src_fdos_kern_fdos_kern_def_h
#define HEADER_fd_src_fdos_kern_fdos_kern_def_h

/* Physical memory layout */

#define FDOS_GPADDR_KERN_IMG   0x1000000UL /* kernel image */
#define FDOS_GPADDR_KERN_HEAP  0x2000000UL /* page table, GDT, TSS, etc */
#define FDOS_GPADDR_KERN_STACK 0x3000000UL /* guest stack */
#define FDOS_GPADDR_USER_MEM   0x4000000UL /* user memory */
#define FDOS_GPADDR_SHMEM      0x5000000UL /* external shared memory */

/* Virtual memory layout */

#define FDOS_GVADDR_SVM_LO                      0x0UL
#define FDOS_GVADDR_SVM_HI              0x100000000UL
#define FDOS_GVADDR_USER_GVCLOCK     0x7ffffffff000UL
#define FDOS_GVADDR_KERN_HEAP    0xffffc90000000000UL
#define FDOS_GVADDR_KERN_STACK   0xffffff8000000000UL
#define FDOS_GVADDR_KERN_IMG     0xffffffff80000000UL

/* Global Descriptor Table */

#define FDOS_GDT_IDX_NULL      0UL
#define FDOS_GDT_IDX_KERN_CODE 1UL
#define FDOS_GDT_IDX_KERN_DATA 2UL
#define FDOS_GDT_IDX_USER_DATA 3UL
#define FDOS_GDT_IDX_USER_CODE 4UL
#define FDOS_GDT_IDX_TSS       5UL
#define FDOS_GDT_IDX_TSS_HIGH  6UL
#define FDOS_GDT_CNT           7UL

#endif /* HEADER_fd_src_fdos_kern_fdos_kern_def_h */
