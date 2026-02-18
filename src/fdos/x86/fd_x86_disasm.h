#ifndef HEADER_fd_fdos_x86_fd_x86_disasm_h
#define HEADER_fd_fdos_x86_fd_x86_disasm_h

#define FD_X86_DISASM_MAX 512

#if FD_HAS_LIBLLVM

#include "../fdos_vmm.h"
#include "../host/fdos_env.h"

char *
fd_x86_disasm( fdos_vmm_alloc_t const * vmm,
               fdos_phys_t const        phys[ FDOS_PIDX_MAX ],
               char                     str[ FD_X86_DISASM_MAX ],
               ulong                    rip );

#endif /* FD_HAS_LIBLLVM */

#endif /* HEADER_fd_fdos_x86_fd_x86_disasm_h */
