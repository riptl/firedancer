#include "fdos_vmm.h"
#include "host/fdos_kvm.h"
#include "../util/fd_util.h"

#include <stddef.h>
#include <errno.h>
#include <stdio.h> /* stderr, fflush */
#include <fcntl.h> /* open(2) */
#include <unistd.h> /* close(2) */
#include <sys/ioctl.h> /* ioctl(2) */
#include <sys/mman.h> /* mmap(2) */

FD_IMPORT( fdos_kern_img, "build/fdos/kern/x86_64/bin/fdos_kern.elf", uchar, 12, "" );

int
main( int     argc,
      char ** argv ) {
  fd_boot( &argc, &argv );

  int flag_trace      = fd_env_strip_cmdline_contains( &argc, &argv, "--trace"           );
  int flag_dump_phys  = fd_env_strip_cmdline_contains( &argc, &argv, "--dump-phys-table" );
  int flag_dump_pt    = fd_env_strip_cmdline_contains( &argc, &argv, "--dump-page-table" );
  int flag_dump_cpuid = fd_env_strip_cmdline_contains( &argc, &argv, "--dump-cpuid"      );
  int flag_init_only  = fd_env_strip_cmdline_contains( &argc, &argv, "--init-only"       );

  /* Create guest kernel data structures */

  fdos_env_t env[1];
  FD_TEST( fdos_env_create( env, fdos_kern_img, fdos_kern_img_sz ) );
  env->trace_mode = flag_trace ? FDOS_TRACE_MODE_RIP : FDOS_TRACE_MODE_OFF;

  /* Create a VM kernel object */

  int kvm_fd = open( "/dev/kvm", O_RDWR|O_CLOEXEC );
  if( FD_UNLIKELY( kvm_fd<0 ) ) {
    FD_LOG_ERR(( "open(/dev/kvm) failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }

  int kvm_version = ioctl( kvm_fd, KVM_GET_API_VERSION, 0 );
  if( FD_UNLIKELY( kvm_version!=KVM_API_VERSION ) ) {
    FD_LOG_ERR(( "Linux KVM version mismatch (have %i, expected %i)", kvm_version, KVM_API_VERSION ));
  }

  int vm_fd = ioctl( kvm_fd, KVM_CREATE_VM, 0 );
  if( FD_UNLIKELY( vm_fd<0 ) ) {
    FD_LOG_ERR(( "KVM_CREATE_VM failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }

  int vcpu_fd = ioctl( vm_fd, KVM_CREATE_VCPU, 0 );
  if( FD_UNLIKELY( vcpu_fd<0 ) ) {
    FD_LOG_ERR(( "KVM_CREATE_VCPU failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }

  /* Install guest kernel state into vCPU */

  fdos_kvm_init( env, kvm_fd, vm_fd, vcpu_fd );

  if( flag_dump_phys ) {
    FD_LOG_NOTICE(( "Guest physical memory map:\n" ));
    for( ulong i=0UL; i<FDOS_PIDX_MAX; i++ ) {
      if( !env->phys[ i ].haddr ) continue;
      FD_LOG_NOTICE(( "  slot=%u gpaddr=%#010x..%#010x haddr=%p",
                      (uint)i, env->phys[ i ].gpaddr0, env->phys[ i ].gpaddr1, (void *)env->phys[ i ].haddr ));
    }
    fputs( "\n", stderr );
    fflush( stderr );
  }

  if( flag_dump_pt ) {
    ulong const * pml4 = (ulong const *)env->pml4;
    FD_LOG_NOTICE(( "Guest page table (at gpaddr=%#lx):\n", env->vmm_alloc->gpaddr ));
    fdos_vmm_printf( pml4, stderr, env->vmm_alloc );
    fputs( "\n", stderr );
    fflush( stderr );
  }

  {
#   define CPUID_MAX 100
    __attribute__((aligned(alignof(struct kvm_cpuid2)))) uchar cpuid_buf[ sizeof(struct kvm_cpuid2) + sizeof(struct kvm_cpuid_entry2) * CPUID_MAX ];
    struct kvm_cpuid2 * cpuid = fd_type_pun( cpuid_buf );
    memset( cpuid, 0, sizeof(cpuid_buf) );
    cpuid->nent = CPUID_MAX;
    if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_CPUID2, cpuid )<0 ) ) {
      FD_LOG_ERR(( "KVM_GET_CPUID2 failed (%i-%s)", errno, fd_io_strerror( errno ) ));
    }
    for( ulong i=0UL; i<cpuid->nent; i++ ) {
      struct kvm_cpuid_entry2 * e = &cpuid->entries[ i ];
      if( flag_dump_cpuid ) {
        FD_LOG_NOTICE(( "CPUID[%08x] eax=%08x ebx=%08x ecx=%08x edx=%08x",
                        e->function, e->eax, e->ebx, e->ecx, e->edx ));
      } else {
        FD_LOG_DEBUG((  "CPUID[%08x] eax=%08x ebx=%08x ecx=%08x edx=%08x",
                        e->function, e->eax, e->ebx, e->ecx, e->edx ));
      }
    }
  }

  /* Map kvm_run struct */

  int mmap_size = ioctl( kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0 );
  if( FD_UNLIKELY( mmap_size<0 ) ) {
    FD_LOG_ERR(( "KVM_GET_VCPU_MMAP_SIZE failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
  struct kvm_run * kvm_run = mmap( NULL, (ulong)mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, vcpu_fd, 0 );
  if( FD_UNLIKELY( kvm_run==MAP_FAILED ) ) {
    FD_LOG_ERR(( "mmap(kvm_run) failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }

  /* Run */

  if( !flag_init_only ) {
    FD_LOG_NOTICE(( "Running KVM guest" ));
    for(;;) {
      if( FD_UNLIKELY( 0!=fdos_kvm_run( env, kvm_run, vcpu_fd ) ) ) break;
    }
    FD_LOG_NOTICE(( "Done" ));
  }

  /* Clean up */

  if( FD_UNLIKELY( munmap( kvm_run, (ulong)mmap_size ) ) ) FD_LOG_ERR(( "munmap(kvm_run) failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  if( FD_UNLIKELY( close( vcpu_fd ) ) ) FD_LOG_ERR(( "close(vcpu) failed (%i-%s)",     errno, fd_io_strerror( errno ) ));
  if( FD_UNLIKELY( close( vm_fd   ) ) ) FD_LOG_ERR(( "close(vm) failed (%i-%s)",       errno, fd_io_strerror( errno ) ));
  if( FD_UNLIKELY( close( kvm_fd  ) ) ) FD_LOG_ERR(( "close(/dev/kvm) failed (%i-%s)", errno, fd_io_strerror( errno ) ));

  fdos_env_destroy( env );

  fd_halt();
  return 0;
}
