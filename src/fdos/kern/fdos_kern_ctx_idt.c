/* Context switching (IDT mode) */

#include "fdos_hypercall.h"

__attribute__((aligned(256)))
__attribute__((naked))
void
fdos_hlt_blob( void ) {
  __asm__ volatile (".space 256, 0xf4");
}

__attribute__((naked))
void
fdos_ring3_enter_idt( ulong user_stack_top_gpaddr, /* rdi */
                      ulong function ) {           /* rsi */
  __asm__ volatile (
    "pushq $0x2b;\n" /* segment 5 */
    "pushq %rdi;\n"  /* user stack */
    "pushq $0x33;\n" /* segment 6 */
    "pushq %rsi;\n"
    "lretq;\n"
  );
}

__attribute__((naked))
__attribute__((noreturn))
void
fdos_kern_entry_idt( fdos_kern_args_t * args ) {
  /* On entry, our GDT, code, and data segment selectors were set up by
     the host.  However, we will need to far return to update the
     descriptor cache.  Otherwise, we would run in the KVM guest default
     state. */

  __asm__ volatile (
    "pushq $0x0;\n"  /* align stack */
    "pushq $0x10;\n" /* FDOS_GDT_IDX_KERN_CS */
    "movabsq $fdos_kern_main, %rax;\n" /* jump target */
    "pushq %rax;\n"
    "lretq;\n"       /* far return (refresh segment selector cache) */
  );
}
