/* fdos_env.c sets up kernel data structures.

   Conventionally, an operating system would set up its own data
   structures in a bootloader.  In fdos, we instead opt to let the host
   set up the kernel's data structures.  This allows the virtualized
   kernel to instantly boot in a ready environment, and also saves
   complex legacy setup (e.g. manually bringing up the CPU from real to
   long mode, or switching between different address spaces). */

#include "fdos_env.h"
#include "fdos_user.h"
#include "../kern/fdos_kern_def.h"
#include "../x86/fd_x86_mmu.h"
#include "../fdos_vmm.h"
#include "../fdos_pvclock.h"
#include <errno.h>
#include <unistd.h>
#include <time.h>

/* fdos_env_tss sets up a dummy Task State Segment */

static void
fdos_env_tss( fdos_env_t * env ) {
  ulong tss_kern_gaddr = fd_wksp_alloc( env->wksp_kern_heap, 16UL, sizeof(fd_x86_tss64_t), 1UL );
  FD_TEST( tss_kern_gaddr ); 

  fd_x86_tss64_t * tss_kern = fd_wksp_laddr_fast( env->wksp_kern_heap, tss_kern_gaddr );

  memset( tss_kern, 0, sizeof(fd_x86_tss64_t) );

  tss_kern->rsp0       = env->stack_kern_top_gvaddr;
  tss_kern->iomap_base = 0x1000; /* exceeds tss_limit -> no IO map */
  tss_kern->ist1       = env->stack_int_top_gvaddr;

  env->tss_kern_gvaddr = FDOS_GVADDR_KERN_HEAP + tss_kern_gaddr;
}

/* fdos_env_gdt sets up the global descriptor table */

static void
fdos_env_gdt( fdos_env_t * env ) {

  ulong gdt_gaddr = fd_wksp_alloc( env->wksp_kern_heap, 16UL, FDOS_GDT_CNT*sizeof(fd_x86_gdt_t), 1UL );
  FD_TEST( gdt_gaddr );
  env->gdt_gvaddr = FDOS_GVADDR_KERN_HEAP + gdt_gaddr;
  fd_x86_gdt_t * gdt = fd_wksp_laddr_fast( env->wksp_kern_heap, gdt_gaddr );
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
  ulong tss_base  = env->tss_kern_gvaddr;
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
  ulong interrupt_handler_gvaddr = env->text.gvaddr;
  env->int_handler_gvaddr = interrupt_handler_gvaddr;

  /* IDT */
  ulong               idt_gaddr  = fd_wksp_alloc( env->wksp_kern_heap, 16UL, 256*sizeof(fd_x86_idt_gate_t), 1UL );
  FD_TEST( idt_gaddr );
  ulong               idt_gvaddr = FDOS_GVADDR_KERN_HEAP + idt_gaddr;
  fd_x86_idt_gate_t * idt        = fd_wksp_laddr_fast( env->wksp_kern_heap, idt_gaddr );  
  for( ulong i=0UL; i<256UL; i++ ) {
    ulong gvaddr = env->text.gvaddr + i;
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
  env->idt_gvaddr = idt_gvaddr;
  env->idt        = idt;
}

static ssize_t
write_kvm( int          fd, 
           void const * buf,
           size_t       count ) {
  fdos_hypercall_write( fd, buf, count );
  return (ssize_t)count; /* FIXME error handling */
}

static int
clock_gettime_kvm( clockid_t         clock_id,
                   struct timespec * tp ) {
  if( FD_UNLIKELY( clock_id!=CLOCK_REALTIME ) ) __asm__ ("hlt");
  fd_pvclock_t * pvclock = (fd_pvclock_t *)FDOS_GVADDR_USER_GVCLOCK;
  long wallclock = fd_pvclock_now( pvclock );
  tp->tv_sec  = wallclock / 1000000000L;
  tp->tv_nsec = wallclock % 1000000000L;
  return 0;
}

static void
patch_trampoline( fdos_env_t * env,
                  ulong        gvaddr,
                  ulong        new_func ) {
  uchar patch[] = {
    0x48, 0xb8, /* movabs rax, imm64 */
    0,0,0,0,0,0,0,0, /* imm64 placeholder */
    0xff, 0xe0  /* jmp rax */
  };
  FD_STORE( ulong, patch+2, new_func );

  ulong gpaddr = fdos_gvaddr_to_gpaddr( gvaddr, sizeof(patch), env->vmm_alloc );
  FD_TEST( gpaddr );
  uchar * haddr = fdos_gpaddr_to_haddr( gpaddr, env->phys );
  FD_TEST( haddr );
  fd_memcpy( haddr, patch, sizeof(patch) );
}

static void
ring3_hello( void ) {
  fd_log_private_logfile_fd_set( 3 );
  fd_log_thread_set( "kvm3" );
  FD_LOG_NOTICE(( "HELLO" ));
}

/* fdos_env_shared sets up interop shared data structures between the
   host and the guest kernel. */

static void
fdos_env_shared( fdos_env_t * env ) {
  /* Entry args */
  ulong entry_args_gaddr = fd_wksp_alloc( env->wksp_kern_heap, alignof(fdos_kern_args_t), sizeof(fdos_kern_args_t), 1UL );
  FD_TEST( entry_args_gaddr );
  env->entry_args_gvaddr = FDOS_GVADDR_KERN_HEAP + entry_args_gaddr;
  env->entry_args        = fd_wksp_laddr_fast( env->wksp_kern_heap, entry_args_gaddr );
  memset( env->entry_args, 0, sizeof(fdos_kern_args_t) );
  fdos_kern_args_t * entry_args = env->entry_args;

  ulong rsp; __asm__ ( "mov %%rsp, %0"      : "=r"(rsp) );
  ulong fs;  __asm__ ( "movq %%fs:0x0, %0"  : "=r"(fs)  );
  entry_args->ring3_fs              = fs;
  entry_args->stack_user_top_gvaddr = rsp;
  entry_args->ring3_entry_gvaddr    = (ulong)ring3_hello;
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

static void
fdos_env_clock_setup( fdos_env_t * env ) {
  ulong pvclock_gaddr = fd_wksp_alloc( env->wksp_kern_heap, FD_SHMEM_NORMAL_PAGE_SZ, fd_ulong_align_up( sizeof(fd_pvclock_t), FD_SHMEM_NORMAL_PAGE_SZ ), 1UL );
  FD_TEST( pvclock_gaddr );
  fd_pvclock_t * pvclock = fd_wksp_laddr_fast( env->wksp_kern_heap, pvclock_gaddr );
  memset( pvclock, 0, sizeof(fd_pvclock_t) );
  env->pvclock             = pvclock;
  env->pvclock_gpaddr      = FDOS_GPADDR_KERN_HEAP + pvclock_gaddr;
  env->pvclock_kern_gvaddr = FDOS_GVADDR_KERN_HEAP + pvclock_gaddr;
  env->pvclock_user_gvaddr = FDOS_GVADDR_USER_GVCLOCK;
  env->entry_args->pvclock_gvaddr = env->pvclock_kern_gvaddr;
  patch_trampoline( env, (ulong)write,         (ulong)write_kvm         );
  patch_trampoline( env, (ulong)clock_gettime, (ulong)clock_gettime_kvm );
}

static void
phys_map_range( fdos_env_t * env,
                uint         slot,
                ulong        gpaddr,
                ulong        haddr,
                ulong        sz ) {
  FD_TEST( slot<FDOS_PIDX_MAX );
  FD_TEST( gpaddr<=UINT_MAX && sz<=UINT_MAX && gpaddr+sz<=UINT_MAX );
  env->phys[ slot ] = (fdos_phys_t) {
    .gpaddr0 = (uint)gpaddr,
    .gpaddr1 = (uint)gpaddr + (uint)sz,
    .haddr   = haddr,
  };
}

static void
phys_map_wksp( fdos_env_t * env,
               uint         slot,
               ulong        gpaddr,
               fd_wksp_t *  wksp ) {
  FD_TEST( slot<FDOS_PIDX_MAX );
  fd_shmem_join_info_t info[1];
  FD_TEST( 0==fd_shmem_join_query_by_join( wksp, info ) );
  phys_map_range( env, slot, gpaddr, (ulong)wksp, info->page_sz * info->page_cnt );
}

fdos_env_t *
fdos_env_create( fdos_env_t *  env,
                 uchar const * kern_bin,
                 ulong         kern_bin_sz ) {
  ulong guest_cpu = fd_log_cpu_id();
  ulong part_max  = 61UL; /* 4096 headroom */

  /* Allocate guest physical memory regions */
  fd_wksp_t * wksp_kern_heap  = fd_wksp_new_anonymous( FD_SHMEM_NORMAL_PAGE_SZ,  1024UL, guest_cpu, "kern_heap",  part_max ); FD_TEST( wksp_kern_heap  );
  fd_wksp_t * wksp_kern_data  = fd_wksp_new_anonymous( FD_SHMEM_NORMAL_PAGE_SZ,  1024UL, guest_cpu, "kern_data",  part_max ); FD_TEST( wksp_kern_data  );
  fd_wksp_t * wksp_kern_stack = fd_wksp_new_anonymous( FD_SHMEM_NORMAL_PAGE_SZ,  1024UL, guest_cpu, "kern_stack", part_max ); FD_TEST( wksp_kern_stack );
  fd_wksp_t * wksp_user_mem   = fd_wksp_new_anonymous( FD_SHMEM_NORMAL_PAGE_SZ, 65536UL, guest_cpu, "user_mem",   part_max ); FD_TEST( wksp_user_mem   );

  /* Guest kernel stack */
  ulong stack_kern_gaddr  = fd_wksp_alloc( wksp_kern_stack, 16UL, 2*FD_SHMEM_HUGE_PAGE_SZ-FD_SHMEM_NORMAL_PAGE_SZ, 1UL );
  FD_TEST( stack_kern_gaddr );

  *env = (fdos_env_t) {
    .wksp_kern_heap   = wksp_kern_heap,
    .wksp_kern_data   = wksp_kern_data,
    .wksp_kern_stack  = wksp_kern_stack,
    .wksp_user_mem    = wksp_user_mem,

    .stack_kern_top_gvaddr = FDOS_GVADDR_KERN_STACK + 2*FD_SHMEM_HUGE_PAGE_SZ - 8192,
    .stack_kern_sz         = 2*FD_SHMEM_HUGE_PAGE_SZ,
    .stack_int_top_gvaddr  = FDOS_GVADDR_KERN_STACK + 2*FD_SHMEM_HUGE_PAGE_SZ - 4096,
    .stack_int_sz          = 4096,
  };

  /* Load kernel image into memory */
  fdos_env_img_load( env, kern_bin, kern_bin_sz );

  /* Set up additional virtual memory mappings */
  ulong * pml4 = (ulong *)env->vmm_alloc->haddr;
  fdos_vmm_map_range(
      pml4,
      FDOS_GVADDR_KERN_HEAP,
      FDOS_GPADDR_KERN_HEAP,
      1024*FD_SHMEM_NORMAL_PAGE_SZ,
      FD_X86_PT_G|FD_X86_PT_RW|FD_X86_PT_XD,
      env->vmm_alloc
  );
  fdos_vmm_map_range(
      pml4,
      FDOS_GVADDR_KERN_STACK,
      FDOS_GPADDR_KERN_STACK,
      1024*FD_SHMEM_NORMAL_PAGE_SZ,
      FD_X86_PT_G|FD_X86_PT_RW|FD_X86_PT_XD,
      env->vmm_alloc
  );

  /* Set up kernel data structures */
  fdos_env_ring0_setup( env );

  /* Set up physical memory mappings */
  phys_map_wksp ( env, FDOS_PIDX_KERN_HEAP,   FDOS_GPADDR_KERN_HEAP,  env->wksp_kern_heap  );
  phys_map_wksp ( env, FDOS_PIDX_KERN_STACK,  FDOS_GPADDR_KERN_STACK, env->wksp_kern_stack );
  phys_map_range( env, FDOS_PIDX_KERN_TEXT,   env->text.gpaddr,       env->text.haddr,   env->text.sz   );
  phys_map_range( env, FDOS_PIDX_KERN_RODATA, env->rodata.gpaddr,     env->rodata.haddr, env->rodata.sz );
  phys_map_range( env, FDOS_PIDX_KERN_DATA,   env->data.gpaddr,       env->data.haddr,   env->data.sz   );
  phys_map_wksp ( env, FDOS_PIDX_USER_MEM,    FDOS_GPADDR_USER_MEM,   env->wksp_user_mem   );

  /* Migrate current userland into VM */
  FD_LOG_NOTICE(( "Migrating userland" ));
  fdos_user_copy( &env->phys[ FDOS_PIDX_USER_MEM ], env->vmm_alloc );

  /* Set up clock */
  fdos_env_clock_setup( env );
  fdos_vmm_map_range(
      pml4,
      FDOS_GVADDR_USER_GVCLOCK,
      env->pvclock_gpaddr,
      FD_SHMEM_NORMAL_PAGE_SZ,
      FD_X86_PT_G|FD_X86_PT_US|FD_X86_PT_XD,
      env->vmm_alloc
  );

  return env;
}

void
fdos_env_destroy( fdos_env_t * env ) {
  fd_wksp_delete_anonymous( env->wksp_kern_heap  );
  fd_wksp_delete_anonymous( env->wksp_kern_data  );
  fd_wksp_delete_anonymous( env->wksp_kern_stack );
  memset( env, 0, sizeof(fdos_env_t) );
}

uchar *
fdos_gpaddr_to_haddr( ulong             gpaddr,
                      fdos_phys_t const phys[ FDOS_PIDX_MAX ] ) {
  ulong phys_idx;
  for( phys_idx=0UL; phys_idx<FDOS_PIDX_MAX; phys_idx++ ) {
    if( !!( gpaddr>=phys[ phys_idx ].gpaddr0 ) &
        !!( gpaddr< phys[ phys_idx ].gpaddr1 ) ) {
      break;
    }
  }
  if( phys_idx==FDOS_PIDX_MAX ) return NULL;     

  ulong off = gpaddr - phys[ phys_idx ].gpaddr0;
  return (uchar *)phys[ phys_idx ].haddr + off;          
}
