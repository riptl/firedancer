/* fdos_kvm.c provides a KVM hypervisor environment for fdos. */

#include "fdos_kvm.h"
#include "../fdos_vmm.h"
#include "../x86/fd_x86_disasm.h"
#include "fdos_env.h"
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h> /* ioctl(2) */

#define TEXT_NORMAL    "\033[0m"
#define TEXT_BOLD      "\033[1m"
#define TEXT_UNDERLINE "\033[4m"
#define TEXT_BLINK     "\033[5m"

#define TEXT_BLUE      "\033[34m"
#define TEXT_GREEN     "\033[32m"
#define TEXT_YELLOW    "\033[93m"
#define TEXT_RED       "\033[31m"

static void *
gvaddr_to_haddr( fdos_env_t const * env,
                 ulong              gvaddr,
                 ulong              sz ) {
  ulong gpaddr = fdos_gvaddr_to_gpaddr( gvaddr, sz, env->vmm_alloc );
  return fdos_gpaddr_to_haddr( gpaddr, sz, env->phys );
}

static void
hypercall_write( fdos_env_t * kern,
                 int          vcpu_fd ) {
  struct kvm_regs regs;
  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_REGS, &regs )<0 ) ) {
    FD_LOG_ERR(( "KVM_GET_REGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
  ulong const * args = gvaddr_to_haddr( kern, regs.rsp, 24UL );
  FD_TEST( args );

  int   fd         = (int)args[ 0 ];
  ulong buf_gvaddr = args[ 1 ];
  ulong len        = args[ 2 ];

  void const * buf = gvaddr_to_haddr( kern, buf_gvaddr, len );
  FD_TEST( buf );

  switch( fd ) {
  case 2:  fd = STDERR_FILENO; break;
  case 3:  fd = fd_log_private_logfile_fd(); break;
  default: FD_LOG_CRIT(( "refusing to write to fd %d", fd ));
  }

  if( fd>=0 ) (void)write( fd, buf, len );
  /* FIXME error handling */
}

static void
fdos_hypercall_handler( fdos_env_t *     env,
                        int              vcpu_fd,
                        struct kvm_run * run ) {
  if( FD_UNLIKELY( run->io.size!=4 || run->io.count!=1 ) ) {
    FD_LOG_CRIT(( "invalid io_out hypercall (size=%u,count=%u)", run->io.size, run->io.count ));
  }
  uint port = run->io.port;
  switch( port ) {
  case FDOS_HYPERCALL_WRITE:
    hypercall_write( env, vcpu_fd );
    break;
  default:
    FD_LOG_CRIT(( "invalid hypercall port %u", port ));
  }
  return;
}

static void
trace_rip( fdos_env_t *     env,
           struct kvm_run * run,
           int              vcpu_fd,
           ulong            rip ) {
  (void)env; (void)run;

  struct kvm_regs regs;
  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_REGS, &regs )<0 ) ) {
    FD_LOG_ERR(( "KVM_GET_REGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
  if( !rip ) rip = regs.rip;

  uchar * code = NULL;
  ulong   rem  = 0UL;

  fdos_phys_t const * phys = env->phys;
  ulong gpaddr = fdos_gvaddr_to_gpaddr( rip, 1UL, env->vmm_alloc );
  for( ulong phys_idx=0UL; phys_idx<FDOS_PIDX_MAX; phys_idx++ ) {
    if( !!( gpaddr>=phys[ phys_idx ].gpaddr0 ) &
        !!( gpaddr< phys[ phys_idx ].gpaddr1 ) ) {
      ulong off = gpaddr - phys[ phys_idx ].gpaddr0;
      code = (uchar *)phys[ phys_idx ].haddr + off;
      rem  = phys[ phys_idx ].gpaddr1 - gpaddr;
    }
  }

  char const * dis = "";
  ulong        cnt = 0UL;
# if FD_HAS_LIBLLVM
  char dis_buf[ FD_X86_DISASM_MAX ];
  cnt = fd_x86_disasm( code, rem, dis_buf, rip );
  if( cnt ) dis = dis_buf;
  else      dis = "                                        ";
# endif

  char hex[ 256 ];
  char * p = fd_cstr_init( hex );
  for( ulong i=0UL; i<cnt; i++ ) {
    static char const hex_tbl[] = "0123456789abcdef";
    p = fd_cstr_append_char( p, hex_tbl[ ( code[ i ] >> 4 ) & 0xf ] );
    p = fd_cstr_append_char( p, hex_tbl[   code[ i ]        & 0xf ] );
    p = fd_cstr_append_char( p, ' ' );
  }
  fd_cstr_fini( p );
  ulong hex_len = (ulong)( p - hex );

  FD_LOG_INFO(( "\033[2mrip=%016lx\033[0m %s \033[2mrsp=%16llx rax=%16llx  %.*s\033[0m",
                rip, dis,
                regs.rsp,
                regs.rax,
                (int)hex_len, hex ));
}

static void
maybe_handle_interrupt( fdos_env_t * kern,
                        int          vcpu_fd,
                        ulong        rip ) {
  ulong hlt0 = kern->text.gvaddr;
  ulong hlt1 = hlt0 + 256;
  if( FD_UNLIKELY( rip<hlt0 || rip>=hlt1 ) ) return;
  uint idx = (uint)( rip - hlt0 );
  if( idx==0x0e ) {
    struct kvm_sregs sregs;
    if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_SREGS, &sregs )<0 ) ) {
      FD_LOG_ERR(( "KVM_GET_REGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
    }
    FD_LOG_NOTICE(( "Page fault address: %#llx", sregs.cr2 ));
  }
  FD_LOG_ERR(( "Caught interrupt type %02x-%s", idx, fd_x86_interrupt_cstr( idx ) ));
}

int
fdos_kvm_run( fdos_env_t *     kern,
              struct kvm_run * kvm_run,
              int              vcpu_fd ) {

  if( kern->trace_mode==FDOS_TRACE_MODE_RIP ) {
    struct kvm_guest_debug debug = {0};
    debug.control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP;
    if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_SET_GUEST_DEBUG, &debug )<0 ) ) {
      FD_LOG_ERR(( "KVM_SET_GUEST_DEBUG failed (%i-%s)", errno, fd_io_strerror( errno ) ));
    }
  }

  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_RUN, 0 ) )<0 ) {
    if( errno==EINTR ) return 0;
    FD_LOG_ERR(( "KVM_RUN failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
  switch( kvm_run->exit_reason ) {
  case KVM_EXIT_IO: /* hypercall */
    if( kvm_run->io.direction==KVM_EXIT_IO_OUT ) {
      fdos_hypercall_handler( kern, vcpu_fd, kvm_run );
    } else {
      FD_LOG_ERR(( "Unexpected INPUT hypercall" ));
    }
    return 0;
  case KVM_EXIT_DEBUG: {
    trace_rip( kern, kvm_run, vcpu_fd, kvm_run->debug.arch.pc );
    return 0;
  }
  case KVM_EXIT_HLT: {
    struct kvm_regs regs;
    if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_REGS, &regs )<0 ) ) {
      FD_LOG_ERR(( "KVM_GET_REGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
    }
    ulong rip = regs.rip - 1UL; /* why is this off by one? */
    maybe_handle_interrupt( kern, vcpu_fd, rip );
    FD_LOG_NOTICE(( "KVM guest issued HLT instruction" ));
    return 1;
  }
  case KVM_EXIT_FAIL_ENTRY:
    FD_LOG_ERR(( "KVM guest failed to enter (hardware_entry_failure_reason=%#llx)", kvm_run->fail_entry.hardware_entry_failure_reason ));
  case KVM_EXIT_SHUTDOWN:
    trace_rip( kern, kvm_run, vcpu_fd, 0UL );
    FD_LOG_WARNING(( "KVM guest shut down (hardware_exit_reason=%#llx)", kvm_run->hw.hardware_exit_reason ));
    return 1;
  case KVM_EXIT_INTERNAL_ERROR:
    FD_LOG_ERR(( "KVM_EXIT_INTERNAL_ERROR (suberror %u)", kvm_run->internal.suberror ));
  default:
    FD_LOG_ERR(( "Unhandled KVM exit reason %u", kvm_run->exit_reason ));
  }
}
