#ifndef HEADER_fd_src_fdos_host_fdos_unicorn_h
#define HEADER_fd_src_fdos_host_fdos_unicorn_h

#include "fdos_env.h"
#include <unicorn/unicorn.h>

void
fdos_unicorn_init( fdos_env_t * env,
                   uc_engine *  uc );

void
fdos_unicorn_run( fdos_env_t * env,
                  uc_engine *  uc );

#endif /* HEADER_fd_src_fdos_host_fdos_unicorn_h */
