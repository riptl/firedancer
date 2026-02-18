/* Entrypoint for FiredancerOS kernel */

#include "fdos_hypercall.h"
#include "../../util/log/fd_log.h"
#include <stdarg.h>
#include <stdio.h>

__attribute__((naked))
__attribute__((section(".text.hlt")))
void
hlt_blob( void ) {
  __asm__ volatile (".space 256, 0xf4");
}

/* fd_util system environment *****************************************/

static fd_hypercall_args_t volatile * g_hyper = NULL;

long
fd_log_wallclock( void ) {
  return (long)fd_tickcount();
}

#define FD_LOG_BUF_SZ (32UL*4096UL)

static char fd_log_private_log_msg[ FD_LOG_BUF_SZ ];
static void
hypercall_log( void ) {
  __asm__ volatile (
    "movw %[port], %%dx;\n"
    "outsl;\n"
    :
    : [port] "r" ((ushort)FDOS_HYPERCALL_LOG)
    : "rdx", "memory"
  );
}

char const *
fd_log_private_0( char const * fmt, ... ) {
  va_list ap;
  va_start( ap, fmt );
  int len = vsnprintf( fd_log_private_log_msg, FD_LOG_BUF_SZ, fmt, ap );
  if( len<0                        ) len = 0;                        /* cmov */
  if( len>(int)(FD_LOG_BUF_SZ-1UL) ) len = (int)(FD_LOG_BUF_SZ-1UL); /* cmov */
  fd_log_private_log_msg[ len ] = '\0';
  va_end( ap );
  return fd_log_private_log_msg;
}

void
fd_log_private_1( int          level,
                  long         now,
                  char const * file,
                  int          line,
                  char const * func,
                  char const * msg ) {
  (void)now;
  fd_hypercall_args_t volatile * args = g_hyper;
  args->log.file_gvaddr = (ulong)file;
  args->log.file_len    = strlen( file );
  args->log.func_gvaddr = (ulong)func;
  args->log.func_len    = strlen( func );
  args->log.msg_gvaddr  = (ulong)msg;
  args->log.msg_len     = strlen( msg );
  args->log.now         = fd_tickcount();
  args->log.line        = line;
  args->log.level       = level;
  hypercall_log();
}

__attribute__((noreturn)) void
fd_log_private_2( int          level,
                  long         now,
                  char const * file,
                  int          line,
                  char const * func,
                  char const * msg ) {
  (void)now;
  fd_log_private_1( level, now, file, line, func, msg );
  __asm__ volatile ("hlt");
  for(;;) {}
}

/* Context switching **************************************************/

__attribute__((naked,noreturn)) void
longjmp( void ) {
  __asm__ volatile (
      "movabsq $g_sysret, %rdi;\n"
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

static ulong
syscall_write( int          fd,
               void const * buf,
               ulong        count ) {
  if( fd==2 && count ) FD_LOG_NOTICE(( "write to fd %d\n%.*s", fd, (int)count-1, (char *)buf ));
  return count;
}

ulong
syscall_handler1( ulong arg0,
                  ulong arg1,
                  ulong arg2,
                  uint  num ) {
  switch( num ) {
  case 1: /* write */
    return syscall_write( (int)arg0, (void *)arg1, arg2 );
  case 231: /* exit_group */
    longjmp();
  default:
    FD_LOG_CRIT(( "unsupported syscall %u", num ));
  }
}

__attribute__((naked)) void
syscall_handler( void ) {
  __asm__ volatile (
      "sub $128, %rsp;\n"
      /* This is probably a bit overkill */
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

struct fd_jmp_buf {
  ulong rbx;
  ulong rbp;
  ulong r12;
  ulong r13;
  ulong r14;
  ulong r15;
  ulong rsp;
  ulong ret;
};

typedef struct fd_jmp_buf fd_jmp_buf_t;

fd_jmp_buf_t g_sysret;

__attribute__((naked)) void
enter_ring3( ulong user_stack_top_gpaddr, /* rdi */
             ulong function,              /* rsi */
             ulong fs ) {                 /* rcx */
  __asm__ volatile (
      "pushq $0x23;\n" /* segment 4 */
      "pushq %rdi;\n"  /* user stack */
      "pushq $0x1b;\n" /* segment 3 */
      "pushq %rsi;\n"
      "movl $0x23, %eax;\n"
      "movw %ax, %ds;\n"
      "movw %ax, %es;\n"
      "wrfsbase %rdx;\n"
      "lretq;\n"
  );
}

__attribute__((naked)) uint
setjmp( void ) {
  __asm__ volatile (
      "movabsq $g_sysret, %rsi;\n"
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

static void
setup_lstar( void ) {
  __asm__ volatile (
      "movl $0xc0000082, %%ecx;\n"
      "movq $syscall_handler, %%rax;\n"
      "movq %%rax, %%rdx;\n"
      "shrq $32, %%rdx;\n"
      "wrmsr;\n"
      :
      : : "rax", "rcx", "rdx", "memory"
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
fdos_kern_main( fdos_kern_args_t * args ) {
  g_hyper = (fd_hypercall_args_t *)args->hyper_args_gvaddr;

  FD_LOG_NOTICE(( "Hello world!" ));

  setup_lstar();

  ulong const user_stack_top_gpaddr = args->stack_user_top_gvaddr-8UL;
  FD_STORE( ulong, (void *)user_stack_top_gpaddr, (ulong)ring3_end );
  if( setjmp()==0 ) {
    enter_ring3( user_stack_top_gpaddr, (ulong)args->ring3_entry_gvaddr, args->ring3_fs );
  } else {
    FD_LOG_NOTICE(( "Returned from ring 3" ));
  }

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
