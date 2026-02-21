#include "fdos_unicorn.h"
#include "../kern/fdos_kern_def.h"
#include "../x86/fd_x86_msr.h"
#include <stddef.h>

static void
uc_map_phys( fdos_env_t * env,
             uc_engine *  uc ) {
  fdos_phys_t const * phys = env->phys;
  for( ulong i=0UL; i<FDOS_PIDX_MAX; i++ ) {
    if( env->phys[ i ].haddr ) continue;
    ulong  addr  = phys[ i ].gpaddr0;
    ulong  sz    = phys[ i ].gpaddr1 - addr;
    uint   perms = UC_PROT_READ | (phys[ i ].rw ? UC_PROT_WRITE : UC_PROT_READ);
    void * ptr   = (void *)phys[ i ].haddr;
    uc_err err = uc_mem_map_ptr( uc, phys[ i ].gpaddr0, phys[ i ].gpaddr1 - phys[ i ].gpaddr0, env->phys[ i ].haddr );
    if( FD_UNLIKELY( err!=UC_ERR_OK ) ) FD_LOG_ERR(( "fail" ));
  }   
}

static void
uc_sregs_set( fdos_env_t * env,
              uc_engine *  uc ) {
  uc_x86_mmr gdt = {
    .base  = env->gdt_gvaddr,
    .limit = (FDOS_GDT_CNT * sizeof(ulong)) - 1U,
  };
  FD_TEST( uc_reg_write( uc, UC_X86_REG_GDTR, &gdt )==UC_ERR_OK );

  uc_x86_mmr idt = {
    .base  = env->idt_gvaddr,
    .limit = (256 * sizeof(fd_x86_idt_gate_t)) - 1U,
  };
  FD_TEST( uc_reg_write( uc, UC_X86_REG_IDTR, &idt )==UC_ERR_OK );

  ushort cs = 0x08;
  ushort ds = 0x10;
  FD_TEST( uc_reg_write( uc, UC_X86_REG_CS, &cs )==UC_ERR_OK );
  FD_TEST( uc_reg_write( uc, UC_X86_REG_DS, &ds )==UC_ERR_OK );
  FD_TEST( uc_reg_write( uc, UC_X86_REG_ES, &ds )==UC_ERR_OK );
  FD_TEST( uc_reg_write( uc, UC_X86_REG_FS, &ds )==UC_ERR_OK );
  FD_TEST( uc_reg_write( uc, UC_X86_REG_GS, &ds )==UC_ERR_OK );
  FD_TEST( uc_reg_write( uc, UC_X86_REG_SS, &ds )==UC_ERR_OK );

  uc_x86_mmr tss = {
    .selector = 0x28,
    .base     = env->tss_kern_gvaddr,
    .limit    = sizeof(fd_x86_tss64_t)-1U,
    .flags    = 0 /* TODO */
  };
  FD_TEST( uc_reg_write( uc, UC_X86_REG_TR, &tss )==UC_ERR_OK );

  /* FIXME disable LDT */

  ulong cr3 = env->vmm_alloc->gpaddr;
  FD_TEST( uc_reg_write( uc, UC_X86_REG_CR3, &cr3 )==UC_ERR_OK );

  ulong cr4 =
      FD_X86_CR4_PAE |
      FD_X86_CR4_PGE |
      FD_X86_CR4_OSFXSR |
      FD_X86_CR4_FSGSBASE |
      FD_X86_CR4_OSXSAVE;
  FD_TEST( uc_reg_write( uc, UC_X86_REG_CR4, &cr4 )==UC_ERR_OK );

  ulong cr0 =
      FD_X86_CR0_PE |
      FD_X86_CR0_MP |
      FD_X86_CR0_ET |
      FD_X86_CR0_NE |
      FD_X86_CR0_WP |
      FD_X86_CR0_AM |
      FD_X86_CR0_PG;
  FD_TEST( uc_reg_write( uc, UC_X86_REG_CR0, &cr0 )==UC_ERR_OK );

  uc_x86_msr efer = {
    .rid = FD_X86_MSR_EFER,
    .value =
      FD_X86_EFER_SCE |
      FD_X86_EFER_LME |
      FD_X86_EFER_LMA |
      FD_X86_EFER_NXE
  };
  FD_TEST( uc_reg_write( uc, UC_X86_REG_MSR, &efer )==UC_ERR_OK );
}

static void
uc_msrs_set( fdos_env_t * env,
             uc_engine *  uc ) {
  
  ulong fs0; __asm__ ( "movq %%fs:0x0, %0" : "=r"(fs0) );
  struct uc_x86_msr fsbase = {
    .rid   = FD_X86_MSR_FSBASE,
    .value = fs0
  };
  FD_TEST( uc_reg_write( uc, UC_X86_REG_MSR, &fsbase )==UC_ERR_OK );

  struct uc_x86_msr msr_star = {
    .rid   = FD_X86_MSR_STAR,
    .value = ((ulong)0x08 << 32) | /* kernel CS */
             ((ulong)0x10 << 48)   /* user CS */
  };
  FD_TEST( uc_reg_write( uc, UC_X86_REG_MSR, &msr_star )==UC_ERR_OK );
  
  struct uc_x86_msr msr_lstar = {
    .rid   = FD_X86_MSR_LSTAR,
    .value = env->text.gvaddr + 256UL
  };
  FD_TEST( uc_reg_write( uc, UC_X86_REG_MSR, &msr_lstar )==UC_ERR_OK );

  struct uc_x86_msr msr_pvclock_epoch = {
    .rid   = FD_X86_MSR_PVCLOCK_EPOCH,
    .value = env->pvclock_gpaddr
  };
  FD_TEST( uc_reg_write( uc, UC_X86_REG_MSR, &msr_pvclock_epoch )==UC_ERR_OK );

  struct uc_x86_msr msr_pvclock_off = {
    .rid   = FD_X86_MSR_PVCLOCK_OFF,
    .value = (env->pvclock_gpaddr + offsetof(fd_pvclock_t, off)) | 1UL
  };
  FD_TEST( uc_reg_write( uc, UC_X86_REG_MSR, &msr_pvclock_off )==UC_ERR_OK );
}

static void
uc_entry_set( fdos_env_t * env,
              uc_engine *  uc ) {
  
}

void
fdos_unicorn_init( fdos_env_t * env,
                   uc_engine *  uc ) {
  uc_map_phys ( env, uc );
  /* FIXME set CPUID */
  uc_sregs_set( env, uc );
  /* FIXME set XCR */
  uc_msrs_set ( env, uc );
  uc_entry_set( env, uc );
}

void
fdos_unicorn_run( fdos_env_t * env,
                  uc_engine *  uc ) {
                  
}
