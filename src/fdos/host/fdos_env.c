/* fdos_env.c sets up kernel data structures.

   Conventionally, an operating system would set up its own data
   structures in a bootloader.  In fdos, we instead opt to let the host
   set up the kernel's data structures.  This allows the virtualized
   kernel to instantly boot in a ready environment, and also saves
   complex legacy setup (e.g. manually bringing up the CPU from real to
   long mode, or switching between different address spaces). */

#include "fdos_env.h"
#include "../kern/fdos_kern_def.h"
#include "../fdos_vmm.h"

/* fdos_env_tss sets up a dummy Task State Segment */

static void
fdos_env_tss( fdos_env_t * env ) {
  ulong tss_kern_gaddr = fd_wksp_alloc( env->wksp_kern_meta, 16UL, sizeof(fd_x86_tss64_t), 1UL );
  ulong tss_user_gaddr = fd_wksp_alloc( env->wksp_kern_meta, 16UL, sizeof(fd_x86_tss64_t), 1UL );
  FD_TEST( tss_kern_gaddr ); FD_TEST( tss_user_gaddr );

  fd_x86_tss64_t * tss_kern = fd_wksp_laddr_fast( env->wksp_kern_meta, tss_kern_gaddr );
  fd_x86_tss64_t * tss_user = fd_wksp_laddr_fast( env->wksp_kern_meta, tss_user_gaddr );

  memset( tss_kern, 0, sizeof(fd_x86_tss64_t) );
  memset( tss_user, 0, sizeof(fd_x86_tss64_t) );

  tss_kern->rsp0       = env->stack_kern_top_gvaddr;
  tss_kern->iomap_base = 0x1000; /* exceeds tss_limit -> no IO map */

  tss_user->rsp0       = env->stack_user_top_gvaddr;
  tss_user->iomap_base = 0x1000; /* exceeds tss_limit -> no IO map */

  env->tss_kern_gpaddr = FDOS_GPADDR_KERN_META + tss_kern_gaddr;
  env->tss_user_gpaddr = FDOS_GPADDR_KERN_META + tss_user_gaddr;
}

/* fdos_env_gdt sets up the global descriptor table */

static void
fdos_env_gdt( fdos_env_t * env ) {

  ulong gdt_gaddr = fd_wksp_alloc( env->wksp_kern_meta, 16UL, FDOS_GDT_CNT*sizeof(fd_x86_gdt_t), 1UL );
  FD_TEST( gdt_gaddr );
  env->gdt_gpaddr = FDOS_GPADDR_KERN_META + gdt_gaddr;
  fd_x86_gdt_t * gdt = fd_wksp_laddr_fast( env->wksp_kern_meta, gdt_gaddr );
  gdt[ FDOS_GDT_IDX_NULL ] = (fd_x86_gdt_t) {0};
  gdt[ FDOS_GDT_IDX_KERN_CODE ] = (fd_x86_gdt_t) {
    .limit0 = 0xffff,
    .base0  = 0,
    .base1  = 0,
    .type   = 11,
    .s      = 1,
    .dpl    = 0,
    .p      = 1,
    .limit1 = 15,
    .avl    = 0,
    .l      = 1,
    .d      = 0,
    .g      = 1,
    .base2  = 0
  };
  gdt[ FDOS_GDT_IDX_KERN_DATA ] = (fd_x86_gdt_t) {
    .limit0 = 0xffff,
    .base0  = 0,
    .base1  = 0,
    .type   = 3,
    .s      = 1,
    .dpl    = 0,
    .p      = 1,
    .limit1 = 15,
    .avl    = 0,
    .l      = 0,
    .d      = 1,
    .g      = 1,
    .base2  = 0
  };
  gdt[ FDOS_GDT_IDX_USER_DATA ] = (fd_x86_gdt_t) {
    .limit0 = 0xffff,
    .base0  = 0,
    .base1  = 0,
    .type   = 11,
    .s      = 1,
    .dpl    = 3,
    .p      = 1,
    .limit1 = 15,
    .avl    = 0,
    .l      = 1,
    .d      = 0,
    .g      = 1,
    .base2  = 0
  };
  gdt[ FDOS_GDT_IDX_USER_CODE ] = (fd_x86_gdt_t) {
    .limit0 = 0xffff,
    .base0  = 0,
    .base1  = 0,
    .type   = 3,
    .s      = 1,
    .dpl    = 3,
    .p      = 1,
    .limit1 = 15,
    .avl    = 0,
    .l      = 0,
    .d      = 1,
    .g      = 1,
    .base2  = 0
  };
  /* TSS */
  ulong tss_base  = env->tss_kern_gpaddr;
  uint  tss_limit = sizeof(fd_x86_tss64_t)-1UL;
  gdt[ FDOS_GDT_IDX_TSS ] = (fd_x86_gdt_t) {
    .limit0 = tss_limit & 0xffffUL,
    .base0  = (ushort)( tss_base & 0xffffUL ),
    .base1  = (tss_base>>16) & 0xffUL,
    .type   = 11,
    .s      = 0,
    .dpl    = 0,
    .p      = 1,
    .limit1 = 0,
    .avl    = 0,
    .l      = 0,
    .d      = 0,
    .g      = 0,
    .base2  = 0
  };
  gdt[ FDOS_GDT_IDX_TSS_HIGH ] = (fd_x86_gdt_t) {
    .base3    = (uint)( tss_base>>32 ),
    .reserved = 0
  };
}

/* fdos_env_idt sets up the interrupt descriptor table */

static void
fdos_env_idt( fdos_env_t * env ) {
  /* Interrupt handler */
  ulong   interrupt_handler_gaddr  = fd_wksp_alloc( env->wksp_kern_code, 256UL, 256UL, 1UL );
  FD_TEST( interrupt_handler_gaddr );
  ulong   interrupt_handler_gvaddr = FDOS_GPADDR_KERN_CODE + interrupt_handler_gaddr;
  uchar * interrupt_handler        = fd_wksp_laddr_fast( env->wksp_kern_code, interrupt_handler_gaddr );
  memset( interrupt_handler, 0xf4, 256UL );
  env->int_handler_gvaddr = interrupt_handler_gvaddr;

  /* IDT */
  ulong               idt_gaddr  = fd_wksp_alloc( env->wksp_kern_meta, 16UL, 256*sizeof(fd_x86_idt_gate_t), 1UL );
  FD_TEST( idt_gaddr );
  ulong               idt_gpaddr = FDOS_GPADDR_KERN_META + idt_gaddr;
  fd_x86_idt_gate_t * idt        = fd_wksp_alloc_laddr( env->wksp_kern_meta, 16UL, 256*sizeof(fd_x86_idt_gate_t), 1UL );
  FD_TEST( idt );
  memset( idt, 0, 256*sizeof(fd_x86_idt_gate_t) );
  for( ulong i=0UL; i<256UL; i++ ) {
    ulong gvaddr = interrupt_handler_gvaddr+i;
    idt[ i ] = (fd_x86_idt_gate_t) {
      .offset_low   = (ushort)( gvaddr & 0xffff ),
      .selector     = 0x08, /* ring 0, GDT, entry 1 (code) */
      .ist          = 0,
      .type_attr    = 0x8e, /* interrupt gate, ring 0, present */
      .offset_mid   = (ushort)((gvaddr >> 16) & 0xffff),
      .offset_high  = (uint)((gvaddr >> 32) & 0xffffffff),
      .reserved     = 0
    };
  }
  env->idt_gpaddr = idt_gpaddr;
  env->idt        = idt;
}

/* fdos_env_shared sets up interop shared data structures between the
   host and the guest kernel. */

static void
fdos_env_shared( fdos_env_t * env ) {

  /* Hypercall shared memory area */
  ulong hyper_args_gaddr = fd_wksp_alloc( env->wksp_kern_data, alignof(fd_hypercall_args_t), sizeof(fd_hypercall_args_t), 1UL );
  FD_TEST( hyper_args_gaddr );
  env->hyper_args_gvaddr = FDOS_GPADDR_KERN_DATA + hyper_args_gaddr;
  env->hyper_args        = fd_wksp_laddr_fast( env->wksp_kern_data, hyper_args_gaddr );
  memset( env->hyper_args, 0, sizeof(fd_hypercall_args_t) );

  /* Entry args */
  ulong entry_args_gaddr = fd_wksp_alloc( env->wksp_kern_data, alignof(fdos_kern_args_t), sizeof(fdos_kern_args_t), 1UL );
  FD_TEST( entry_args_gaddr );
  env->entry_args_gvaddr = FDOS_GPADDR_KERN_DATA + entry_args_gaddr;
  env->entry_args        = fd_wksp_laddr_fast( env->wksp_kern_data, entry_args_gaddr );
  memset( env->entry_args, 0, sizeof(fdos_kern_args_t) );
  fdos_kern_args_t * entry_args = env->entry_args;

  entry_args->hyper_args_gvaddr     = env->hyper_args_gvaddr;
  entry_args->stack_user_top_gvaddr = env->stack_user_top_gvaddr;
}

/* fdos_env_ring0_setup sets up various dynamic x86 data structures and
   hypervisor interop logic */

static void
fdos_env_ring0_setup( fdos_env_t * env ) {
  fdos_env_tss   ( env );
  fdos_env_gdt   ( env );
  fdos_env_idt   ( env );
  fdos_env_shared( env );
}

fdos_env_t *
fdos_env_create( fdos_env_t *  env,
                 uchar const * kern_bin,
                 ulong         kern_bin_sz ) {
  ulong guest_cpu = fd_log_cpu_id();
  ulong part_max  = 61UL; /* 4096 headroom */

  /* Allocate guest physical memory regions */
  fd_wksp_t * wksp_kern_meta   = fd_wksp_new_anonymous( FD_SHMEM_HUGE_PAGE_SZ, 2UL, guest_cpu, "guest_meta",   part_max ); FD_TEST( wksp_kern_meta   );
  fd_wksp_t * wksp_kern_code   = fd_wksp_new_anonymous( FD_SHMEM_HUGE_PAGE_SZ, 2UL, guest_cpu, "guest_code",   part_max ); FD_TEST( wksp_kern_code   );
  fd_wksp_t * wksp_kern_rodata = fd_wksp_new_anonymous( FD_SHMEM_HUGE_PAGE_SZ, 2UL, guest_cpu, "guest_rodata", part_max ); FD_TEST( wksp_kern_rodata );
  fd_wksp_t * wksp_kern_data   = fd_wksp_new_anonymous( FD_SHMEM_HUGE_PAGE_SZ, 2UL, guest_cpu, "guest_data",   part_max ); FD_TEST( wksp_kern_data   );
  fd_wksp_t * wksp_kern_stack  = fd_wksp_new_anonymous( FD_SHMEM_HUGE_PAGE_SZ, 2UL, guest_cpu, "guest_stack",  part_max ); FD_TEST( wksp_kern_stack  );
  fd_wksp_t * wksp_user_stack  = fd_wksp_new_anonymous( FD_SHMEM_HUGE_PAGE_SZ, 2UL, guest_cpu, "user_stack",   part_max ); FD_TEST( wksp_user_stack  );

  /* Root page */
  ulong   pml4_gaddr  = fd_wksp_alloc( wksp_kern_meta, FD_SHMEM_NORMAL_PAGE_SZ, FD_SHMEM_NORMAL_PAGE_SZ, 1UL ); FD_TEST( pml4_gaddr );
  ulong   pml4_gpaddr = FDOS_GPADDR_KERN_META + pml4_gaddr;
  ulong * pml4        = fd_wksp_laddr_fast( wksp_kern_meta, pml4_gaddr );
  memset( pml4, 0, FD_SHMEM_NORMAL_PAGE_SZ );

  /* Create page table */
  fdos_vmm_map_range( pml4, wksp_kern_meta, FDOS_GPADDR_KERN_META,   2UL*FD_SHMEM_HUGE_PAGE_SZ, 0 );
  fdos_vmm_map_range( pml4, wksp_kern_meta, FDOS_GPADDR_KERN_STACK,  2UL*FD_SHMEM_HUGE_PAGE_SZ, 0 );
  fdos_vmm_map_range( pml4, wksp_kern_meta, FDOS_GPADDR_USER_STACK,  2UL*FD_SHMEM_HUGE_PAGE_SZ, 1 );

  /* Guest kernel stack */
  ulong stack_kern_gaddr  = fd_wksp_alloc( wksp_kern_stack, 16UL, 2*FD_SHMEM_HUGE_PAGE_SZ-FD_SHMEM_NORMAL_PAGE_SZ, 1UL );
  FD_TEST( stack_kern_gaddr );
  ulong stack_kern_gpaddr = FDOS_GPADDR_KERN_STACK + 2*FD_SHMEM_HUGE_PAGE_SZ;

  /* Guest user stack */
  ulong stack_user_gaddr  = fd_wksp_alloc( wksp_user_stack, 16UL, 2*FD_SHMEM_HUGE_PAGE_SZ-FD_SHMEM_NORMAL_PAGE_SZ, 1UL );
  FD_TEST( stack_user_gaddr );
  ulong stack_user_gpaddr = FDOS_GPADDR_USER_STACK + 2*FD_SHMEM_HUGE_PAGE_SZ;

  *env = (fdos_env_t) {
    .wksp_kern_meta   = wksp_kern_meta,
    .wksp_kern_code   = wksp_kern_code,
    .wksp_kern_rodata = wksp_kern_rodata,
    .wksp_kern_data   = wksp_kern_data,
    .wksp_kern_stack  = wksp_kern_stack,
    .wksp_user_stack  = wksp_user_stack,

    .pml4        = pml4,
    .pml4_gpaddr = pml4_gpaddr,

    .stack_kern_top_gvaddr = stack_kern_gpaddr,
    .stack_kern_sz         = stack_kern_gpaddr-FDOS_GPADDR_KERN_STACK-stack_kern_gaddr,

    .stack_user_top_gvaddr = stack_user_gpaddr,
    .stack_user_sz         = stack_user_gpaddr-FDOS_GPADDR_USER_STACK-stack_user_gaddr
  };

  /* Load kernel image into memory */
  fdos_env_img_load( env, kern_bin, kern_bin_sz );

  /* Set up kernel data structures */
  fdos_env_ring0_setup( env );

  return env;
}

void
fdos_env_destroy( fdos_env_t * env ) {
  fd_wksp_delete_anonymous( env->wksp_kern_meta   );
  fd_wksp_delete_anonymous( env->wksp_kern_code   );
  fd_wksp_delete_anonymous( env->wksp_kern_rodata );
  fd_wksp_delete_anonymous( env->wksp_kern_data   );
  fd_wksp_delete_anonymous( env->wksp_kern_stack  );
  memset( env, 0, sizeof(fdos_env_t) );
}
