/* Context switching (FRED mode) */

#include "fdos_hypercall.h"

/* FiredancerOS currently forwards all FRED events (including
   faulting interrupts) to the KVM host.

   Registers are clobbered because these conditions are not
   recoverable (host does a bit of debugging, then terminates
   the VM). */

__attribute__((aligned(4096)))
__attribute__((naked)) void
fdos_fred_handler( void ) {
  __asm__ volatile (
    /* from ring 3 */
    "movq 0(%rsp), %r12;\n"  /* error code */
    "movq 8(%rsp), %r13;\n"  /* rip */
    "movq 40(%rsp), %r14;\n" /* event info (incl type) */
    "movq 48(%rsp), %r15;\n" /* event data */
    // /* Re-enable single stepping, works around bug in KVM FRED patch v9 */
    // "pushfq;\n"
    // "pop %r11;\n"
    // "orq $0x100, %r11;\n"
    // "push %r11;\n"
    // "popfq;\n"
    /* Detect syscalls */
    "shrq $48, %r14;\n"
    "andl $0x0f, %r14d;\n"
    "cmp $7, %r14d;\n"
    "je fdos_syscall_handler;\n"
    /* Not a syscall ... */
    "movq 40(%rsp), %r14;\n" /* event info (incl type) */
    "hlt;\n"
    // "eretu;\n"
    ".align 256;\n"

    /* from ring 0 */
    "movq 0(%rsp), %r12;\n"  /* error code */
    "movq 8(%rsp), %r13;\n"  /* rip */
    "movq 40(%rsp), %r14;\n" /* event info (incl type) */
    "movq 48(%rsp), %r15;\n" /* event data */
    "hlt;\n"
    // "erets;\n"
  );
}

__attribute__((naked))
void
fdos_ring3_enter_fred( ulong user_stack_top_gpaddr, /* rdi */
                       ulong function ) {           /* rsi */
  __asm__ volatile (
    "pushq $0x2b;\n"   /* stack segment selector (4) */
    "pushq %rdi;\n"    /* user stack pointer */
    "pushq $0x202;\n"  /* rflags */
    "pushq $0x33;\n"   /* code segment selector (5) */
    "pushq %rsi;\n"    /* jump target */
    "pushq $0;\n"      /* error code (skipped by ERETU) */
    "eretu;\n"
  );
}

__attribute__((noreturn))
void
fdos_kern_entry_fred( fdos_kern_args_t * args ) {

  (void)args;
  __asm__ volatile (
    "movabsq $fdos_kern_main, %rax;\n"
    "jmp *%rax;\n"
  );

  // /* On entry, our GDT, code, and data segment selectors were set up by
  //    the host.  However, we will need to far return to update the
  //    descriptor cache.  Otherwise, we would run in the KVM guest default
  //    state. */

  // __asm__ volatile (
  //   "pushq $0x10;\n"    /* stack segment selector (2), ring 0 */
  //   "pushq %%rsp;\n"    /* kernel stack pointer */
  //   "pushq $0x3004;\n"  /* rflags */
  //   "pushq $0x08;\n"    /* code segment selector (1), ring 0 */
  //   "movabsq $fdos_kern_main, %%rax;\n"
  //   "pushq %%rax;\n"    /* jump target */
  //   "movq %0, %%rdi;\n" /* first argument */
  //   "erets;\n"
  //   : : "r" (args) : "rax", "rdi", "memory"
  // );

  __builtin_unreachable();
}
