#ifndef HEADER_fd_src_fdos_host_fdos_user_h
#define HEADER_fd_src_fdos_host_fdos_user_h

#include <unistd.h>
#include <time.h>

void
fdos_user_entrypoint( void );

/* Userland symbol overrides */

ssize_t
fdos_user_write( int          fd,
                 void const * buf,
                 size_t       count );

int
fdos_user_clock_gettime( clockid_t         clock_id,
                         struct timespec * tp );

#endif /* HEADER_fd_src_fdos_host_fdos_user_h */
