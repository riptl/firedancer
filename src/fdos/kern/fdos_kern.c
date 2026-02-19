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
      "jne .slow_path;\n"
      "movq $0xffffff80003fe008, %rbp;\n"
      "movq %rbp, %rsp;\n"
      "jmp ring0_entrypoint;\n"
      /* This is probably a bit overkill */
      ".slow_path:"
      "sub $128, %rsp;\n"
      "pushq %rbp;\n"
      "pushq %rdi;\n"
      "pushq %rsi;\n"
      "pushq %rbx;\n"
      "pushq %rcx;\n"
      "pushq %rdx;\n"
      "pushq %r8;\n"
      "pushq %r9;\n"
      "pushq %r10;\n"
      "pushq %r12;\n"
      "pushq %r13;\n"
      "pushq %r14;\n"
      "pushq %r15;\n"
      "movq %rax, %rcx;\n"
      "movq %rsp, %rbp;\n"
      "and $-16, %rsp;\n"
      "callq syscall_handler1;\n"
      "movq %rbp, %rsp;\n"
      "popq %r15;\n"
      "popq %r14;\n"
      "popq %r13;\n"
      "popq %r12;\n"
      "popq %r10;\n"
      "popq %r9;\n"
      "popq %r8;\n"
      "popq %rdx;\n"
      "popq %rcx;\n"
      "popq %rbx;\n"
      "popq %rsi;\n"
      "popq %rdi;\n"
      "popq %rbp;\n"
      "add $128, %rsp;\n"
      "sysretq;\n"
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

static fdos_kern_args_t * g_args;

__attribute__((noreturn)) void
ring0_entrypoint( void );

ulong
syscall_handler1( ulong arg0,
                  ulong arg1,
                  ulong arg2,
                  uint  num ) {
  (void)arg2;
  switch( num ) {
  case 257: /* openat */
    /* FIXME validate arg1 pointer */
    FD_LOG_CRIT(( "rejected userland syscall 'openat(%d,%s)', aborting", (int)arg0, (char const *)arg1 ));
  default:
    __asm__ ("hlt" );
    FD_LOG_CRIT(( "unsupported syscall %u", num ));
  }
}

/* ring3 API */

__attribute__((naked,noreturn)) void
ring3_enter( ulong stack,       /* rdi */
             ulong function ) { /* rsi */
  __asm__ volatile (
      "pushq $0x23;\n" /* segment 4 */
      "pushq %rdi;\n"
      "pushq $0x1b;\n" /* segment 3 */
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

__attribute__((noreturn)) void
ring0_entrypoint( void ) {
  static ulong iter = 0UL;
  static long start;
  if( FD_UNLIKELY( !iter ) ) {
    FD_LOG_NOTICE(( "Benchmarking" ));
    start = fd_log_wallclock();
  }
  else if( FD_UNLIKELY( iter>(ulong)1e7 ) ) {
    FD_LOG_ERR(( "Done (%lu ns/iter)", (ulong)( (double)(fd_log_wallclock()-start)/(double)iter ) ));
  }
  iter++;
  fdos_kern_args_t * args = g_args;
  ulong const ustack = args->stack_user_top_gvaddr-8UL;
  ulong const uentry = args->ring3_entry_gvaddr;
  ring3_enter( ustack, uentry );
}

__attribute__((noreturn)) void
fdos_kern_main( fdos_kern_args_t * args ) {
  g_args = args;
  g_pvclock = (fd_pvclock_t *)args->pvclock_gvaddr;

  fd_log_thread_set( "kvm0" );
  fd_log_wallclock_set( fd_pvclock_now, g_pvclock );
  fd_log_colorize_set( 1 );

  ulong const ustack = args->stack_user_top_gvaddr-8UL;
  ulong const uentry = args->ring3_entry_gvaddr;
  FD_STORE( ulong, (void *)ustack, (ulong)ring3_end );
  ring3_enter( ustack, uentry );
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
