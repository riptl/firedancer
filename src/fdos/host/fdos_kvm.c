/* fdos_kvm.c provides a KVM hypervisor environment for fdos. */

#include "fdos_kvm.h"
#include "../fdos_vmm.h"
#include "../x86/fd_x86_disasm.h"
#include "../x86/fd_x86_msr.h"
#include "fdos_env.h"
#include <errno.h>
#include <stdio.h>
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
      break;
    }
  }

  char const * dis = "";
  ulong        cnt = 0UL;
# if FD_HAS_LIBLLVM
  char dis_buf[ FD_X86_DISASM_MAX ];
  cnt = fd_x86_disasm( code, rem, dis_buf, rip );
  if( cnt ) dis = dis_buf;
  else      dis = "                                        ";
# else
  (void)rem;
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

  fprintf( stderr, "\033[2mrip=%016lx\033[0m %s \033[2mrsp=%16llx  %.*s\033[0m\n",
           rip, dis,
           regs.rsp,
           (int)hex_len, hex );
}

static void
maybe_handle_fred_event( fdos_env_t * env,
                         int          vcpu_fd,
                         ulong        rip ) {
  ulong hlt0 = env->fred_handler_gvaddr;
  ulong hlt1 = hlt0 + 4096;
  if( FD_UNLIKELY( rip<hlt0 || rip>=hlt1 ) ) return;

  struct kvm_regs regs;
  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_REGS, &regs )<0 ) ) {
    FD_LOG_ERR(( "KVM_GET_REGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
  __attribute__((aligned(alignof(struct kvm_msrs))))
  uchar msrs_buf[ sizeof(struct kvm_msrs) + sizeof(struct kvm_msr_entry) ];
  struct kvm_msrs * msr_req = fd_type_pun( msrs_buf );
  msr_req->nmsrs = 1;
  msr_req->entries[ 0 ].index = FD_X86_MSR_FRED_CONFIG;
  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_MSRS, msr_req )<0 ) ) {
    FD_LOG_ERR(( "KVM_GET_MSRS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }

  _Bool ring3      = rip < hlt0+256;
  uint  error_code = regs.r12 & 0xfff;
  ulong fault_rip  = regs.r13;
  uint  evtype     = ( regs.r14 >> 48 ) & 0x0f;
  uint  idx        = ( regs.r14 >> 32 ) & 0xff;
  uint  instr_len  = ( regs.r14 >> 60 );
  ulong event_data = regs.r15;
  ulong csl        = msr_req->entries[ 0 ].data & 0x3;

  FD_LOG_NOTICE(( "Registers:\n"
                  "  rax=%016llx rbx=%016llx rcx=%016llx rdx=%016llx\n"
                  "  rsi=%016llx rdi=%016llx rsp=%016llx rbp=%016llx\n"
                  "  r8 =%016llx r9 =%016llx r10=%016llx r11=%016llx",
                  regs.rax, regs.rbx, regs.rcx, regs.rdx,
                  regs.rsi, regs.rdi, regs.rsp, regs.rbp,
                  regs.r8,  regs.r9,  regs.r10, regs.r11 ));
  FD_LOG_ERR(( "Caught FRED event (stack level %lu)\n"
               "  event_type=%d-%s\n"
               "  vector=%02x-%s\n"
               "  fault_address=%#lx (%u bytes, ring %c)\n"
               "  event_data=%#lx\n"
               "  error_code=%06x",
               csl,
               evtype, fd_x86_evtype_cstr( evtype ),
               idx,    fd_x86_interrupt_cstr( idx ),
               fault_rip, instr_len, ring3?'3':'0',
               event_data,
               error_code ));
}

static void
maybe_handle_interrupt( fdos_env_t * env,
                        int          vcpu_fd,
                        ulong        rip ) {
  ulong hlt0 = env->int_handler_gvaddr;
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

  struct kvm_regs regs;
  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_REGS, &regs )<0 ) ) {
    FD_LOG_ERR(( "KVM_GET_REGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
  FD_LOG_NOTICE(( "Registers:\n"
                  "  rax=%016llx rbx=%016llx rcx=%016llx rdx=%016llx\n"
                  "  rsi=%016llx rdi=%016llx rsp=%016llx rbp=%016llx\n"
                  "  r8 =%016llx r9 =%016llx r10=%016llx r11=%016llx\n"
                  "  r12=%016llx r13=%016llx r14=%016llx r15=%016llx\n",
                  regs.rax, regs.rbx, regs.rcx, regs.rdx,
                  regs.rsi, regs.rdi, regs.rsp, regs.rbp,
                  regs.r8,  regs.r9,  regs.r10, regs.r11,
                  regs.r12, regs.r13, regs.r14, regs.r15 ));
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
    ulong pc = kvm_run->debug.arch.pc;
    trace_rip( kern, kvm_run, vcpu_fd, pc );
    return 0;
  }
  case KVM_EXIT_HLT: {
    struct kvm_regs regs;
    if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_REGS, &regs )<0 ) ) {
      FD_LOG_ERR(( "KVM_GET_REGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
    }
    ulong rip = regs.rip - 1UL; /* why is this off by one? */
    if( kern->fred ) maybe_handle_fred_event( kern, vcpu_fd, rip );
    else             maybe_handle_interrupt ( kern, vcpu_fd, rip );
    FD_LOG_NOTICE(( "KVM guest issued HLT instruction" ));
    return 1;
  }
  case KVM_EXIT_FAIL_ENTRY:
    FD_LOG_ERR(( "KVM guest failed to enter (hardware_entry_failure_reason=%#llx)", kvm_run->fail_entry.hardware_entry_failure_reason ));
  case KVM_EXIT_SHUTDOWN:
    trace_rip( kern, kvm_run, vcpu_fd, 0UL );
    FD_LOG_WARNING(( "KVM guest shut down (hardware_exit_reason=%#llx)", kvm_run->hw.hardware_exit_reason ));
    return 1;
  case KVM_EXIT_MMIO:
    trace_rip( kern, kvm_run, vcpu_fd, 0UL );
    FD_LOG_CRIT(( "Invalid physical memory access by KVM guest (phys_addr=%#llx, len=%u, r%c)", kvm_run->mmio.phys_addr, kvm_run->mmio.len, kvm_run->mmio.is_write?'w':'r' ));
  case KVM_EXIT_INTERNAL_ERROR:
    FD_LOG_ERR(( "KVM_EXIT_INTERNAL_ERROR (suberror %u)", kvm_run->internal.suberror ));
  default:
    FD_LOG_ERR(( "Unhandled KVM exit reason %u", kvm_run->exit_reason ));
  }
}
