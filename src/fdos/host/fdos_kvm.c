/* fdos_kvm.c provides a KVM hypervisor environment for fdos. */

#include "fdos_kvm.h"
#include "../fdos_vmm.h"
#include "../x86/fd_x86_disasm.h"
#include "fdos_env.h"
#include <errno.h>
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
  ulong gpaddr0 = fdos_gvaddr_to_gpaddr( gvaddr, sz, env->vmm_alloc );
  ulong gpaddr1 = gpaddr0 + sz;

  ulong phys;
  for( phys=0UL; phys<FDOS_PIDX_MAX; phys++ ) {
    if( !!( gpaddr0>=env->phys[ phys ].gpaddr0 ) &
        !!( gpaddr1<=env->phys[ phys ].gpaddr1 ) ) {
      break;
    }
  }
  if( FD_UNLIKELY( phys==FDOS_PIDX_MAX ) ) return NULL;

  ulong off = gpaddr0 - env->phys[ phys ].gpaddr0;
  return (void *)( env->phys[ phys ].haddr + off );
}

static void
hypercall_log( fdos_env_t *                kern,
               int                         vcpu_fd,
               fd_hypercall_args_t const * args ) {
  /* FIXME don't use cstr for translation ... stupid */
  char const * file = (char const *)gvaddr_to_haddr( kern, args->log.file_gvaddr, args->log.file_len );
  char const * func = (char const *)gvaddr_to_haddr( kern, args->log.func_gvaddr, args->log.func_len );
  char const * msg  = (char const *)gvaddr_to_haddr( kern, args->log.msg_gvaddr,  args->log.msg_len  );

  struct kvm_sregs sregs;
  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_SREGS, &sregs )<0 ) ) {
    FD_LOG_ERR(( "KVM_GET_REGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }

  long now = fd_log_wallclock();
  char   thread_backup[ FD_LOG_NAME_MAX ];
  char * thread_name = (char *)fd_log_thread();
  memcpy( thread_backup, thread_name, FD_LOG_NAME_MAX );
  strcpy( thread_name, "kvm" );
  fd_log_private_1( (int)args->log.level, now, file, (int)args->log.line, func, msg );
  memcpy( thread_name, thread_backup, FD_LOG_NAME_MAX );
}

void
fdos_hypercall_handler( fdos_env_t *     env,
                        int              vcpu_fd,
                        struct kvm_run * run ) {
  if( FD_UNLIKELY( run->io.size!=4 || run->io.count!=1 ) ) {
    FD_LOG_CRIT(( "invalid io_out hypercall (size=%u,count=%u)", run->io.size, run->io.count ));
  }
  fd_hypercall_args_t const * args = env->hyper_args;
  uint port = run->io.port;
  switch( port ) {
  case FDOS_HYPERCALL_LOG:
    hypercall_log( env, vcpu_fd, args );
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

  char const * dis = "";
# if FD_HAS_LIBLLVM
  char dis_buf[ FD_X86_DISASM_MAX ];
  dis = fd_x86_disasm(
      env->vmm_alloc,
      env->phys,
      dis_buf,
      rip
  );
  if( !dis ) dis = "                                        ";
# endif

  FD_LOG_INFO(( "\033[2mrip=%016lx\033[0m %s \033[2mrsp=%16llx rax=%16llx rbx=%16llx rcx=%16llx rdx=%16llx rsi=%16llx rdi=%16llx\033[0m",
                rip, dis,
                regs.rsp, regs.rax, regs.rbx, regs.rcx, regs.rdx, regs.rsi, regs.rdi ));
}

int
fdos_kvm_run( fdos_env_t *     kern,
              struct kvm_run * kvm_run,
              int              vcpu_fd ) {
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
  case KVM_EXIT_HLT:
    FD_LOG_NOTICE(( "KVM guest issued HLT instruction" ));
    return 1;
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
