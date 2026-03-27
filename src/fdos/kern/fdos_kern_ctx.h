#ifndef HEADER_fd_src_fdos_kern_fdos_kern_ctx_h
#define HEADER_fd_src_fdos_kern_fdos_kern_ctx_h

#include "../../util/fd_util_base.h"

/* Kernel-internal APIs for context switching */

FD_PROTOTYPES_BEGIN

/* fdos_ring3_enter transitions to ring 3.
   Assumes caller is in ring 0.
   Sets rsp and rip to the given arguments. */

extern void (* fdos_ring3_enter_ptr)( ulong new_rsp, ulong new_rip );

__attribute__((naked))
__attribute__((noreturn))
static inline void
fdos_ring3_enter( ulong new_rsp,    /* in rdi */
                  ulong new_rip ) { /* in rsi */
  __asm__ volatile (
    "jmp *fdos_ring3_enter_ptr(%rip);\n"
  );
}

/* fdos_ring3_exit transitions back to ring 0.
   Assumes caller is in ring 3 (entered via fdos_ring3_enter).
   rsp is left untouched, rdi is set to the syscall handler. */

__attribute__((noreturn))
__attribute__((naked))
FD_FN_UNUSED static void
fdos_ring3_exit( void ) {
  __asm__ volatile (
    "mov $231, %eax;\n"
    "syscall;\n"
    "ud2;\n"
  );
}

FD_PROTOTYPES_END

#endif /* HEADER_fd_src_fdos_kern_fdos_kern_ctx_h */
