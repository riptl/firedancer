#ifndef HEADER_fd_src_fdos_host_fdos_migrate_h
#define HEADER_fd_src_fdos_host_fdos_migrate_h

#include "fdos_env.h"
#include "../fdos_vmm.h"

void
fdos_migrate_self( fdos_phys_t *      phys,
                   fdos_vmm_alloc_t * alloc );

#endif /* HEADER_fd_src_fdos_host_fdos_migrate_h */
