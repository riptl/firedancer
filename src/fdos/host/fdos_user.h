#ifndef HEADER_fd_src_fdos_host_fdos_user_h
#define HEADER_fd_src_fdos_host_fdos_user_h

#include "fdos_env.h"
#include "../fdos_vmm.h"

void
fdos_user_copy( fdos_phys_t *      phys,
                fdos_vmm_alloc_t * alloc );

#endif /* HEADER_fd_src_fdos_host_fdos_user_h */
