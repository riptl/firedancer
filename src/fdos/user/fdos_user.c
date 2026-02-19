#include "fdos_user.h"
#include "../kern/fdos_hypercall.h"
#include "../kern/fdos_kern_def.h"
#include "../fdos_pvclock.h"
#include "../../util/fd_util.h"

__attribute__((noreturn)) void
fdos_user_exit_group( int status ) {
  __asm__ volatile (
      "mov $231, %%eax;\n"
      "syscall;\n"
      "ud2;\n"
      :: "D" (status)
  );
  __builtin_unreachable();
}

ssize_t
fdos_user_write( int          fd,
                 void const * buf,
                 size_t       count ) {
  /* Used by fd_log, cannot use FD_LOG_* (recursion) */
  fdos_hypercall_write( fd, buf, count );
  return (ssize_t)count; /* FIXME error handling */
}

int
fdos_user_clock_gettime( clockid_t         clock_id,
                         struct timespec * tp ) {
  /* Used by fd_log, cannot use FD_LOG_* (recursion) */
  if( FD_UNLIKELY( clock_id!=CLOCK_REALTIME ) ) __asm__ ("hlt");
  fd_pvclock_t * pvclock = (fd_pvclock_t *)FDOS_GVADDR_USER_GVCLOCK;
  long wallclock = fd_pvclock_now( pvclock );
  tp->tv_sec  = wallclock / 1000000000L;
  tp->tv_nsec = wallclock % 1000000000L;
  return 0;
}

void
fdos_user_entrypoint( void ) {
  FD_ONCE_BEGIN {
    fd_log_private_logfile_fd_set( 3 );
    fd_log_thread_set( "kvm3" );
    FD_LOG_NOTICE(( "HELLO" ));
  }
  FD_ONCE_END;
}
