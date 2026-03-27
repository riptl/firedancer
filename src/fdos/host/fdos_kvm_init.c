#include "fdos_env.h"
#include "fdos_kvm.h"
#include "fdos_cpuid.h"
#include "../kern/fdos_kern_def.h"
#include "../x86/fd_x86_msr.h"
#include <stddef.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/kvm.h>

static void
vm_caps_set( int vm_fd ) {
  if( ioctl( vm_fd, KVM_ENABLE_CAP, &(struct kvm_enable_cap) {
      .cap = KVM_CAP_X86_TRIPLE_FAULT_EVENT
  } )<0 ) {
    FD_LOG_ERR(( "KVM_ENABLE_CAP(KVM_CAP_X86_TRIPLE_FAULT_EVENT) failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
}

static ulong
vcpu_cpuid_set( int kvm_fd,
                int vcpu_fd ) {
# define CPUID_MAX 100
  __attribute__((aligned(alignof(struct kvm_cpuid2)))) uchar cpuid_buf[ sizeof(struct kvm_cpuid2) + sizeof(struct kvm_cpuid_entry2) * CPUID_MAX ];
  struct kvm_cpuid2 * cpuid = fd_type_pun( cpuid_buf );
  memset( cpuid, 0, sizeof(cpuid_buf) );
  cpuid->nent = CPUID_MAX;
  if( FD_UNLIKELY( ioctl( kvm_fd, KVM_GET_SUPPORTED_CPUID, cpuid )<0 ) ) {
    FD_LOG_ERR(( "KVM_GET_SUPPORTED_CPUID failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }

  fdos_cpuid_check_t check[1];
  fdos_cpuid_check_init( check );
  for( ulong i=0UL; i<cpuid->nent; i++ ) {
    struct kvm_cpuid_entry2 * e = &cpuid->entries[ i ];
    fdos_cpuid_check_push( check, e->function, e->index, e->eax, e->ebx, e->ecx, e->edx );
  }
  fdos_cpuid_validate( check );

  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_SET_CPUID2, cpuid )<0 ) ) {
    FD_LOG_ERR(( "KVM_SET_CPUID2 failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
  return check->cpu_feat;
}

static void
vcpu_sregs_set( fdos_env_t * env,
                int          vcpu_fd ) {

  struct kvm_sregs sregs[1];
  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_SREGS, sregs )<0 ) ) {
    FD_LOG_ERR(( "KVM_GET_SREGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }

  /* GDT */

  sregs->gdt.base  = env->gdt_gvaddr;
  sregs->gdt.limit = (FDOS_GDT_CNT * sizeof(ulong)) - 1UL;
  memset( sregs->gdt.padding, 0, sizeof(sregs->gdt.padding) );

  /* Segment descriptors */

  struct kvm_segment cs = {
    .base     = 0,
    .limit    = 0xffffffff,
    .selector = 0x08, /* ring 0, GDT, entry 1 (code) */
    .present  = 1,
    .type     = 0xb,
    .dpl      = 0,
    .db       = 0,
    .s        = 1,
    .l        = 1,
    .g        = 1
  };
  sregs->cs = cs;
  struct kvm_segment ds = {
    .base     = 0,
    .limit    = 0xffffffff,
    .selector = 0x10, /* ring 0, GDT, entry 2 (data) */
    .type     = 0x3,
    .present  = 1,
    .dpl      = 0,
    .db       = 0,
    .s        = 1,
    .l        = 1,
    .g        = 1
  };
  sregs->ds = ds;
  sregs->es = ds;
  sregs->fs = ds;
  sregs->gs = ds;
  sregs->ss = ds;

  /* IDT */

  if( !env->fred ) {
    sregs->idt.base  = env->idt_gvaddr;
    sregs->idt.limit = (256 * sizeof(fd_x86_idt_gate_t)) - 1UL;
  }

  /* TSS */

  sregs->tr.base     = env->tss_kern_gvaddr;
  sregs->tr.limit    = FD_X86_TSS64_FULL_SZ - 1UL;
  sregs->tr.selector = 0x28;
  sregs->tr.type     = 0xb;
  sregs->tr.present  = 1;
  sregs->tr.dpl      = 0;
  sregs->tr.s        = 0;
  sregs->tr.g        = 0;

  sregs->ldt.unusable = 1;

  /* Enable long mode */

  sregs->cr3 = (ulong)env->vmm_alloc->gpaddr;
  FD_TEST( fd_ulong_is_aligned( sregs->cr3, FD_SHMEM_NORMAL_PAGE_SZ ) );
  sregs->cr4 =
      FD_X86_CR4_PAE |
      FD_X86_CR4_PGE |
      FD_X86_CR4_OSFXSR |
      FD_X86_CR4_FSGSBASE |
      FD_X86_CR4_OSXSAVE |
      ( env->fred ? FD_X86_CR4_FRED : 0 );

  sregs->cr0 =
      FD_X86_CR0_PE |
      FD_X86_CR0_MP |
      FD_X86_CR0_ET |
      FD_X86_CR0_NE |
      FD_X86_CR0_WP |
      FD_X86_CR0_AM |
      FD_X86_CR0_PG;

  sregs->efer =
      FD_X86_EFER_SCE |
      FD_X86_EFER_LME |
      FD_X86_EFER_LMA |
      FD_X86_EFER_NXE;

  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_SET_SREGS, sregs )<0 ) ) {
    FD_LOG_ERR(( "KVM_SET_SREGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }

  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_SET_SREGS, sregs )<0 ) ) {
    FD_LOG_ERR(( "KVM_SET_SREGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
}

static void
vcpu_xcrs_set( int   vcpu_fd,
               ulong cpu_features ) {
  struct kvm_xcrs xcrs;
  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_GET_XCRS, &xcrs )<0 ) ) {
    FD_LOG_ERR(( "KVM_GET_XCRS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
  for( ulong i=0UL; i<xcrs.nr_xcrs; i++ ) {
    if( xcrs.xcrs[ i ].xcr==0 ) {
      xcrs.xcrs[ i ].value |= FD_X86_XCR0_X87;
      if( cpu_features & FDOS_CPU_FEAT_REG_XMM ) {
        xcrs.xcrs[ i ].value |= FD_X86_XCR0_SSE;
      }
      if( cpu_features & FDOS_CPU_FEAT_REG_YMM ) {
        xcrs.xcrs[ i ].value |= FD_X86_XCR0_AVX;
      }
      if( cpu_features & FDOS_CPU_FEAT_REG_ZMM ) {
        xcrs.xcrs[ i ].value |= FD_X86_XCR0_OPMASK | FD_X86_XCR0_ZMM_HI256 | FD_X86_XCR0_HI16_ZMM;
      }
      break;
    }
  }
  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_SET_XCRS, &xcrs )<0 ) ) {
    FD_LOG_ERR(( "KVM_SET_XCRS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
}

static void
vcpu_msrs_set( fdos_env_t * env,
               int          vcpu_fd ) {
# define MSR_CNT 11
  __attribute__((aligned(alignof(struct kvm_msrs))))
  uchar msrs_buf[ sizeof(struct kvm_msrs) + MSR_CNT*sizeof(struct kvm_msr_entry) ];

  struct kvm_msrs * msr_req = fd_type_pun( msrs_buf );
  msr_req->nmsrs = MSR_CNT;
  ulong i = 0UL;

  /* FS base */

  ulong fs0; __asm__ ( "movq %%fs:0x0, %0" : "=r"(fs0) );
  msr_req->entries[ i   ].index = FD_X86_MSR_FSBASE;
  msr_req->entries[ i++ ].data  = fs0;

  /* SYSCALL configuration */

  ulong msr_star = (( (FDOS_GDT_IDX_KERN_CS <<3)   )<<32) | /* sysret cs base -> kernel CS */
                   (( (FDOS_GDT_IDX_U32_CODE<<3)|3 )<<48);  /* user CS */
  msr_req->entries[ i   ].index = FD_X86_MSR_STAR;
  msr_req->entries[ i++ ].data  = msr_star;

  ulong msr_lstar = env->syscall_handler_gvaddr;
  msr_req->entries[ i   ].index = FD_X86_MSR_LSTAR;
  msr_req->entries[ i++ ].data  = msr_lstar;

  /* KVM clock */

  msr_req->entries[ i   ].index = FD_X86_MSR_PVCLOCK_EPOCH;
  msr_req->entries[ i++ ].data  = env->pvclock_gpaddr;
  msr_req->entries[ i   ].index = FD_X86_MSR_PVCLOCK_OFF;
  msr_req->entries[ i++ ].data  = (env->pvclock_gpaddr + offsetof(fd_pvclock_t, off)) | 1UL;

  /* FRED stack pointers */

  if( env->fred ) {

    msr_req->entries[ i   ].index = FD_X86_MSR_FRED_CONFIG;
    msr_req->entries[ i++ ].data  = env->entry_fred_gvaddr;

    msr_req->entries[ i   ].index = FD_X86_MSR_FRED_STKLVLS;
    msr_req->entries[ i++ ].data  = 0UL;

    msr_req->entries[ i   ].index = FD_X86_MSR_FRED_RSP0;
    msr_req->entries[ i++ ].data  = env->stack_int_top_gvaddr;

    msr_req->entries[ i   ].index = FD_X86_MSR_FRED_RSP1;
    msr_req->entries[ i++ ].data  = env->stack_int_top_gvaddr;

    msr_req->entries[ i   ].index = FD_X86_MSR_FRED_RSP2;
    msr_req->entries[ i++ ].data  = env->stack_int_top_gvaddr;

    msr_req->entries[ i   ].index = FD_X86_MSR_FRED_RSP3;
    msr_req->entries[ i++ ].data  = env->stack_int_top_gvaddr;

  }

  FD_TEST( i<=MSR_CNT );
# undef MSR_CNT

  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_SET_MSRS, msr_req )<0 ) ) {
    FD_LOG_ERR(( "KVM_SET_MSRS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
}

static void
vcpu_entry_set( fdos_env_t * env,
                int          vcpu_fd ) {
  struct kvm_regs regs = {
    .rdi    = env->entry_args_gvaddr,
    .rip    = env->entry_gvaddr,
    .rflags = FD_X86_RFLAGS_PF | FD_X86_RFLAGS_IOPL3,
    .rsp    = env->stack_kern_top_gvaddr,
    .rbp    = env->stack_kern_top_gvaddr
  };
  if( FD_UNLIKELY( ioctl( vcpu_fd, KVM_SET_REGS, &regs )<0 ) ) {
    FD_LOG_ERR(( "KVM_SET_REGS failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }
  FD_LOG_INFO(( "Initial CPU state: rip=%#llx rsp=%#llx rdi=%#llx",
                regs.rip, regs.rsp, regs.rdi ));
}

static void
vm_map_phys( fdos_env_t * env,
             int          vm_fd ) {
  fdos_phys_t const * phys = env->phys;
  for( ulong i=0UL; i<FDOS_PIDX_MAX; i++ ) {
    if( !env->phys[ i ].haddr ) continue;
    struct kvm_userspace_memory_region region = {
      .slot            = (uint)i,
      .guest_phys_addr = phys[ i ].gpaddr0,
      .memory_size     = phys[ i ].gpaddr1 - phys[ i ].gpaddr0,
      .userspace_addr  = phys[ i ].haddr,
      .flags           = phys[ i ].rw ? 0 : KVM_MEM_READONLY
    };

    if( FD_UNLIKELY( ioctl( vm_fd, KVM_SET_USER_MEMORY_REGION, &region )<0 ) ) {
      FD_LOG_ERR(( "KVM_SET_USER_MEMORY_REGION(slot=%u,guest_phys_addr=%#llx,memory_size=%#llx,userspace_addr=%p) failed (%i-%s)",
                  region.slot, region.guest_phys_addr, region.memory_size, (void *)region.userspace_addr, errno, fd_io_strerror( errno ) ));
    }
  }
}

void
fdos_kvm_init( fdos_env_t * env,
               int          kvm_fd,
               int          vm_fd,
               int          vcpu_fd ) {
  vm_map_phys   ( env, vm_fd );
  vm_caps_set   ( vm_fd );
  ulong cpu_features = vcpu_cpuid_set( kvm_fd, vcpu_fd );
  env->fred = !!( cpu_features & FDOS_CPU_FEAT_FRED );
  fdos_env_img_patch( env );
  vcpu_sregs_set( env, vcpu_fd );
  vcpu_xcrs_set ( vcpu_fd, cpu_features );
  vcpu_msrs_set ( env, vcpu_fd );
  vcpu_entry_set( env, vcpu_fd );
}
