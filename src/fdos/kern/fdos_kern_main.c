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
      "cmp $231, %eax;\n"
      "je longjmp;\n"
      "ud2;\n"
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

/* Context switching **************************************************/

__attribute__((aligned(64))) ulong g_save[ 128 ];

__attribute__((naked)) uint
setjmp( void ) {
  __asm__ volatile (
      "movabsq $g_save, %rsi;\n"
      "movq %rbx, (%rsi);\n"
      "movq %rbp, 8(%rsi);\n"
      "movq %r12, 16(%rsi);\n"
      "movq %r13, 24(%rsi);\n"
      "movq %r14, 32(%rsi);\n"
      "movq %r15, 40(%rsi);\n"
      "leaq 8(%rsp), %rdx;\n"
      "movq %rdx, 48(%rsi);\n"
      "movq (%rsp), %rdx;\n"
      "movq %rdx, 56(%rsi);\n"
      "xorl %eax, %eax;\n"
      "retq;\n"
  );
}

__attribute__((naked,noreturn)) void
longjmp( void ) {
  __asm__ volatile (
      "movabsq $g_save, %rdi;\n"
      "movq (%rdi), %rbx;\n"
      "movq 8(%rdi), %rbp;\n"
      "movq 16(%rdi), %r12;\n"
      "movq 24(%rdi), %r13;\n"
      "movq 32(%rdi), %r14;\n"
      "movq 40(%rdi), %r15;\n"
      "movq 48(%rdi), %rsp;\n"
      "jmp *56(%rdi);\n"
  );
}

static void
farcall_ring3( ulong stack_top_gvaddr,
               ulong func ) {
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

  if( setjmp()==0 ) {
    fdos_ring3_enter( stack_top_gvaddr, func );
    __builtin_unreachable();
  }

  pml4[ 0 ] = 0UL;
  pml3[ 0 ] = 0UL;
  pml2[ 0 ] = 0UL;
  pml1[ 1 ] = 0UL;
  ulong descriptor[ 2 ] = { 0UL, 0UL };
  _invpcid( 3, descriptor ); /* invalidate TLB except global pages */
}


__attribute__((noreturn)) void
fdos_kern_main( fdos_kern_args_t * args ) {
  g_pvclock   = (fd_pvclock_t *)args->pvclock_gvaddr;
  g_vmm_alloc = args->vmm_alloc;

  fd_log_thread_set( "kvm0" );
  fd_log_wallclock_set( fd_pvclock_now, g_pvclock );
  fd_log_colorize_set( 1 );

  FD_LOG_NOTICE(( "Hello world!" ));
  ulong const ustack = args->stack_user_top_gvaddr-8UL;
  ulong const uentry = args->ring3_entry_gvaddr;
  FD_STORE( ulong, (void *)ustack, (ulong)fdos_ring3_exit );

  farcall_ring3( ustack, uentry );
  FD_LOG_NOTICE(( "Returned from ring 3" ));

  FD_LOG_NOTICE(( "Benchmarking" ));
  long dt = -fd_log_wallclock();
  ulong iter = (ulong)1e7;
  for( ulong i=0UL; i<iter; i++ ) {
    farcall_ring3( ustack, uentry );
  }
  dt += fd_log_wallclock(); (void)dt;
  FD_LOG_NOTICE(( "Context switching: %lu ns/iter", (ulong)( (double)dt/(double)iter ) ));

  FD_LOG_ERR(( "Goodbye" ));
}
