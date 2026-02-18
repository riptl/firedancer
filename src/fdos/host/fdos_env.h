#ifndef HEADER_fd_src_fdos_host_fdos_host_h
#define HEADER_fd_src_fdos_host_fdos_host_h

#include "../kern/fdos_hypercall.h"
#include "../fdos_vmm.h"
#include "../fdos_pvclock.h"
#include "../x86/fd_x86_gdt.h"
#include "../x86/fd_x86_idt.h"
#include "../x86/fd_x86_tss.h"
#include "../../util/wksp/fd_wksp.h"

struct fdos_vmo {
  ulong haddr;
  ulong gvaddr;
  ulong gpaddr;
  ulong sz;
};

typedef struct fdos_vmo fdos_vmo_t;

#define FDOS_PIDX_KERN_HEAP   0
#define FDOS_PIDX_KERN_STACK  1
#define FDOS_PIDX_KERN_TEXT   2
#define FDOS_PIDX_KERN_RODATA 3
#define FDOS_PIDX_KERN_DATA   4
#define FDOS_PIDX_USER_MEM    5
#define FDOS_PIDX_MAX         6

struct fdos_phys {
  uint  gpaddr0;
  uint  gpaddr1;
  ulong haddr;
};

typedef struct fdos_phys fdos_phys_t;

struct fdos_env {
  fd_wksp_t * wksp_kern_heap;  /* general-purpose heap allocator */
  fd_wksp_t * wksp_kern_data;  /* .data section */
  fd_wksp_t * wksp_kern_stack;
  fd_wksp_t * wksp_user_mem;   /* copy of user virtual memory map */

  /* Physical memory mappings */
  fdos_phys_t phys[ FDOS_PIDX_MAX ];

  /* Page tables
     First page is PML4, various other pages follow */
  fdos_vmm_alloc_t vmm_alloc[1];

  /* Kernel stack */
  ulong stack_kern_top_gvaddr;
  ulong stack_kern_sz;
  ulong stack_int_top_gvaddr;
  ulong stack_int_sz;

  /* Kernel image */
  fdos_vmo_t text;
  fdos_vmo_t rodata;
  fdos_vmo_t data;
  ulong      entry_gvaddr;

  /* TSS (kernel, user) */
  fd_x86_tss64_t * tss_kern;
  ulong            tss_kern_gvaddr;

  /* GDT */
  ulong          gdt_gvaddr;
  fd_x86_gdt_t * gdt;

  /* Default interrupt handler */
  ulong int_handler_gvaddr; /* 256 bytes, 1 byte for each interrupt descriptor */

  /* IDT */
  ulong               idt_gvaddr;
  fd_x86_idt_gate_t * idt;

  /* Startup args */
  ulong              entry_args_gvaddr;
  fdos_kern_args_t * entry_args;

  /* pvclock */
  fd_pvclock_t * pvclock;
  ulong          pvclock_gpaddr;
  ulong          pvclock_kern_gvaddr;
  ulong          pvclock_user_gvaddr;
};

typedef struct fdos_env fdos_env_t;

void
fdos_env_img_load( fdos_env_t *  env,
                   uchar const * bin,
                   ulong         bin_sz );

/* fdos_env_create sets up all fdos kernel data structures
   needed to bootstrap a KVM ring 0 guest environment. */

fdos_env_t *
fdos_env_create( fdos_env_t *  env,
                 uchar const * kern_bin,
                 ulong         kern_bin_sz );

void
fdos_env_destroy( fdos_env_t * env );

uchar *
fdos_gpaddr_to_haddr( ulong             gpaddr,
                      fdos_phys_t const phys[ FDOS_PIDX_MAX ] );

#endif /* HEADER_fd_src_fdos_host_fdos_host_h */
