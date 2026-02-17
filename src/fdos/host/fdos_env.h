#ifndef HEADER_fd_src_fdos_host_fdos_host_h
#define HEADER_fd_src_fdos_host_fdos_host_h

#include "../kern/fdos_hypercall.h"
#include "../fdos_vmm.h"
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

#define FDOS_PHYS_MAX 16UL

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
  fd_wksp_t * wksp_user_stack;

  /* Physical memory mappings */
  fdos_phys_t phys[ FDOS_PHYS_MAX ];

  /* Page tables
     First page is PML4, various other pages follow */
  fdos_vmm_alloc_t vmm_alloc[1];

  /* Stack (kernel, user) */
  ulong   stack_kern_top_gvaddr;
  ulong   stack_kern_sz;
  ulong   stack_user_top_gvaddr;
  ulong   stack_user_sz;

  /* Kernel image */
  fdos_vmo_t text;
  fdos_vmo_t rodata;
  fdos_vmo_t data;
  ulong      entry_gvaddr;

  /* TSS (kernel, user) */
  fd_x86_tss64_t * tss_kern;
  fd_x86_tss64_t * tss_user;
  ulong            tss_kern_gpaddr;
  ulong            tss_user_gpaddr;

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

  /* Hypercalls */
  ulong                 hyper_args_gvaddr;
  fd_hypercall_args_t * hyper_args;
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

#endif /* HEADER_fd_src_fdos_host_fdos_host_h */
