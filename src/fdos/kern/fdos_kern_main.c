#include "fdos_hypercall.h"
#include "fdos_kern_def.h"
#include "fdos_kern_ctx.h"
#include "../fdos_pvclock.h"
#include "../fdos_vmm.h"
#include "../x86/fd_x86_mmu.h"
#include "../../util/log/fd_log.h"
#include <immintrin.h>

__attribute__((naked)) void
fdos_syscall_handler( void ) {
  __asm__ volatile (
      "movabs $0xffffff80003ffff8UL, %rax\n"
      "mov %rax, %rsp\n"
      "jmp fdos_kern_step;\n"
  );
}

void (* fdos_ring3_enter_ptr)( ulong new_rsp, ulong new_rip )
  = (__typeof__(fdos_ring3_enter_ptr))0x41414141;

/* fd_util system environment *****************************************/

static fd_pvclock_t *   g_pvclock;
static fdos_vmm_alloc_t g_vmm_alloc;

void
fd_log_flush( void ) {}

int
fd_io_write( int          fd,
             void const * src,
             ulong        src_min,
             ulong        src_max,
             ulong *      _src_sz ) {
  (void)src_min;
  fdos_hypercall_write( fd, src, src_max );
  *_src_sz = src_max; /* FIXME error handling */
  return 0;
}

__attribute__((noreturn))
static void
farcall_ring3( ulong stack_top_gvaddr,
               ulong func ) {
  fdos_ring3_enter( stack_top_gvaddr, func );
  __builtin_unreachable();
}

static long  bench_start;
static ulong iter_rem = 1e7;
static ulong ustack;
static ulong uentry;

__attribute__((noreturn))
void
fdos_kern_step( void ) {
  if( FD_UNLIKELY( !iter_rem-- ) ) {
    ulong iter = (ulong)1e8;
    long dt = fd_log_wallclock() - bench_start;
    FD_LOG_NOTICE(( "Context switching: %lu ns/iter", (ulong)( (double)dt/(double)iter ) ));
    FD_LOG_ERR(( "Goodbye" ));
  }

  fdos_vmm_alloc_t * alloc = &g_vmm_alloc;
  ulong * pml4 = (ulong *)alloc->haddr;
  ulong   next = alloc->next;

  /* Fast variant of vmm_map_range */
  // fdos_vmm_map_range( pml4, 0x1000UL, FDOS_GPADDR_SHMEM, 4096UL, FD_X86_PT_US|FD_X86_PT_RW, alloc );
  ulong   pml3_gpaddr    = alloc->gpaddr + next;
  ulong   pml2_gpaddr    = alloc->gpaddr + next +   FD_X86_PM_SZ;
  ulong   pml1_gpaddr    = alloc->gpaddr + next + 2*FD_X86_PM_SZ;
  ulong * pml3 = (ulong *)( alloc->haddr + next                  );
  ulong * pml2 = (ulong *)( alloc->haddr + next +   FD_X86_PM_SZ );
  ulong * pml1 = (ulong *)( alloc->haddr + next + 2*FD_X86_PM_SZ );
  FD_ONCE_BEGIN {
    void * out = pml3;
    ulong  sz  = 3*FD_X86_PM_SZ;
    __asm__ __volatile__( "rep stosb" : "+D" (out), "+c" (sz) : "a" (0x00) : "memory" );
  }
  FD_ONCE_END;

  pml4[ 0 ] = pml3_gpaddr | FD_X86_PT_P | FD_X86_PT_RW | FD_X86_PT_US;
  pml3[ 0 ] = pml2_gpaddr | FD_X86_PT_P | FD_X86_PT_RW | FD_X86_PT_US;
  pml2[ 0 ] = pml1_gpaddr | FD_X86_PT_P | FD_X86_PT_RW | FD_X86_PT_US;
  pml1[ 1 ] = FDOS_GPADDR_SHMEM | FD_X86_PT_P | FD_X86_PT_RW | FD_X86_PT_US | FD_X86_PT_XD;

  ulong descriptor[ 2 ] = { 0UL, 0UL };
  _invpcid( 3, descriptor ); /* invalidate TLB except global pages */
  farcall_ring3( ustack, uentry );
}


__attribute__((noreturn)) void
fdos_kern_main( fdos_kern_args_t * args ) {
  g_pvclock   = (fd_pvclock_t *)args->pvclock_gvaddr;
  g_vmm_alloc = args->vmm_alloc;

  fd_log_thread_set( "kvm0" );
  fd_log_wallclock_set( fd_pvclock_now, g_pvclock );
  fd_log_colorize_set( 1 );

  FD_LOG_NOTICE(( "Hello world!" ));
  ustack = args->stack_user_top_gvaddr-8UL;
  uentry = args->ring3_entry_gvaddr;
  FD_STORE( ulong, (void *)ustack, (ulong)fdos_ring3_exit );

  bench_start = fd_log_wallclock();
  FD_LOG_NOTICE(( "Benchmarking" ));
  fdos_kern_step();
}
