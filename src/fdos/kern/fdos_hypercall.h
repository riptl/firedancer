#ifndef HEADER_fd_src_fdos_kern_fdos_hypercall_h
#define HEADER_fd_src_fdos_kern_fdos_hypercall_h

/* fdos_hypercall.h provides the ABI for kernel-to-host hypercalls.
   This mechanism is used by the fdos kernel to sychronously send
   messages to the KVM host (e.g. logging). */

#include "../../util/fd_util_base.h"

/* kernel boot parameters */

struct fdos_kern_args {
  ulong stack_user_top_gvaddr;
  ulong ring3_entry_gvaddr;
  ulong ring3_fs;
  ulong pvclock_gvaddr;
};

typedef struct fdos_kern_args fdos_kern_args_t;

/* FDOS_HYPERCALL_* give hypercall IDs. */

#define FDOS_HYPERCALL_WRITE 1

static inline void
fdos_hypercall_write( int          fd,
                      void const * buf,
                      ulong        len ) {
  __asm__ volatile (
    "movw $" FD_EXPAND_THEN_STRINGIFY( FDOS_HYPERCALL_WRITE ) ", %%dx;\n"
    "pushq %[len];\n"
    "pushq %[buf];\n"
    "pushq %[fd];\n"
    "movq %%rsp, %%rsi;\n"
    "outsl;\n"
    "addq $24, %%rsp;\n"
    :
    : [fd] "r" ((ulong)fd), [buf] "r" (buf), [len] "r" (len)
    : "rsi", "rdx", "memory"
  );
}

#endif /* HEADER_fd_src_fdos_kern_fdos_hypercall_h */
