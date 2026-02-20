/* Entrypoint for FiredancerOS kernel */

#include "fdos_hypercall.h"
#include "../fdos_pvclock.h"
#include "../x86/fd_x86_msr.h"
#include "../../util/log/fd_log.h"
#include <stdarg.h>
#include <immintrin.h>

__attribute__((section(".text.hlt")))
__attribute__((naked))
void
hlt_blob( void ) {
  __asm__ volatile (".space 256, 0xf4");
}

__attribute__((section(".text.syscall")))
__attribute__((naked)) void
syscall_handler( void ) {
  __asm__ volatile (
      "cmp $231, %eax;\n"
      "je longjmp;\n"
      "ud2;\n"
  );
}

/* fd_util system environment *****************************************/

static fd_pvclock_t * g_pvclock;

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

/* ring3 API */

__attribute__((naked)) void
ring3_enter( ulong user_stack_top_gpaddr, /* rdi */
             ulong function ) {           /* rsi */
  __asm__ volatile (
      "pushq $0x1b;\n" /* segment 3 */
      "pushq %rdi;\n"  /* user stack */
      "pushq $0x23;\n" /* segment 4 */
      "pushq %rsi;\n"
      "lretq;\n"
  );
}

__attribute__((naked)) static void
ring3_end( void ) {
  __asm__ volatile (
      "mov $231, %eax;\n"
      "syscall;\n"
      "ud2;\n"
  );
}

static void
ring3_run( ulong stack_top_gvaddr,
           ulong func ) {
  if( setjmp()==0 ) {
    ring3_enter( stack_top_gvaddr, func );
    __builtin_unreachable();
  }
}

__attribute__((noreturn)) void
fdos_kern_main( fdos_kern_args_t * args ) {
  g_pvclock = (fd_pvclock_t *)args->pvclock_gvaddr;

  fd_log_thread_set( "kvm0" );
  fd_log_wallclock_set( fd_pvclock_now, g_pvclock );
  fd_log_colorize_set( 1 );

  FD_LOG_NOTICE(( "Hello world!" ));
  ulong const ustack = args->stack_user_top_gvaddr-8UL;
  ulong const uentry = args->ring3_entry_gvaddr;
  FD_STORE( ulong, (void *)ustack, (ulong)ring3_end );

  ring3_run( ustack, uentry );
  FD_LOG_NOTICE(( "Returned from ring 3" ));

  FD_LOG_NOTICE(( "Benchmarking" ));
  long dt = -fd_log_wallclock();
  ulong iter = (ulong)1e7;
  for( ulong i=0UL; i<iter; i++ ) {
    ring3_run( ustack, uentry );
  }
  dt += fd_log_wallclock(); (void)dt;
  FD_LOG_NOTICE(( "Context switching: %lu ns/iter", (ulong)( (double)dt/(double)iter ) ));

  FD_LOG_ERR(( "Goodbye" ));
}

__attribute__((noreturn)) void
fdos_kern_entry( fdos_kern_args_t * args ) {

  /* On entry, our GDT, code, and data segment selectors were set up by
     the host.  However, we will need to far return to make the content
     of these structures take changes.  Otherwise, we would run in some
     undocumented KVM guest default state. */

  __asm__ volatile (
      /* Select segment 1, privilege level 0 */
      "pushq $8;\n"
      /* Return address (entry1) */
      "movabsq $fdos_kern_main, %%rax;\n"
      "pushq %%rax;\n"
      /* First argument to entry1 */
      "movq %0, %%rdi;\n"
      /* Far return */
      "lretq;\n"
      : : "r" (args) : "rax", "rdi", "memory"
  );

  __builtin_unreachable();
}
