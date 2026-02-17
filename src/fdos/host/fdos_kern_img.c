#include "fdos_env.h"
#include "../kern/fdos_kern_def.h"
#include "../fdos_vmm.h"
#include "../x86/fd_x86_mmu.h"
#include "../../ballet/elf/fd_elf64.h"

/* Kernel loader */

struct fdos_kern_img_off {
  fd_elf64_ehdr ehdr;
  fd_elf64_phdr phdr_rom;
  fd_elf64_phdr phdr_code;
  fd_elf64_phdr phdr_ram;
};

typedef struct fdos_kern_img_off fdos_kern_img_off_t;

static fdos_kern_img_off_t *
fdos_kern_img_off_load( fdos_kern_img_off_t * img_off,
                        uchar const *         bin,
                        ulong                 bin_sz ) {
  memset( img_off, 0, sizeof(fdos_kern_img_off_t) );

  img_off->ehdr = FD_LOAD( fd_elf64_ehdr, bin );
  FD_TEST( FD_LOAD( uint, img_off->ehdr.e_ident )==fd_uint_bswap( 0x7f454c46 ) );
  /* FIXME More validation */
  FD_TEST( img_off->ehdr.e_phnum==3 );
  FD_TEST( img_off->ehdr.e_phentsize==sizeof(fd_elf64_phdr) );

  ulong phdr_off0 = img_off->ehdr.e_phoff;
  ulong phdr_off1 = img_off->ehdr.e_phoff + img_off->ehdr.e_phnum*sizeof(fd_elf64_phdr);
  FD_TEST( phdr_off1<=bin_sz );

  uchar const * phdr_i = bin + phdr_off0;
  img_off->phdr_rom  = FD_LOAD( fd_elf64_phdr, phdr_i ); phdr_i += sizeof(fd_elf64_phdr);
  img_off->phdr_code = FD_LOAD( fd_elf64_phdr, phdr_i ); phdr_i += sizeof(fd_elf64_phdr);
  img_off->phdr_ram  = FD_LOAD( fd_elf64_phdr, phdr_i ); phdr_i += sizeof(fd_elf64_phdr);

  FD_TEST( img_off->phdr_rom .p_type  == FD_ELF_PT_LOAD );
  FD_TEST( img_off->phdr_code.p_type  == FD_ELF_PT_LOAD );
  FD_TEST( img_off->phdr_ram .p_type  == FD_ELF_PT_LOAD );

  FD_TEST( img_off->phdr_rom .p_vaddr >= FDOS_GVADDR_KERN_IMG );
  FD_TEST( img_off->phdr_code.p_vaddr >= FDOS_GVADDR_KERN_IMG );
  FD_TEST( img_off->phdr_ram .p_vaddr >= FDOS_GVADDR_KERN_IMG );

  FD_TEST( img_off->phdr_rom .p_flags == 4 );
  FD_TEST( img_off->phdr_code.p_flags == 5 );
  FD_TEST( img_off->phdr_ram. p_flags == 6 );

  FD_TEST( img_off->phdr_rom .p_filesz == img_off->phdr_rom .p_memsz );
  FD_TEST( img_off->phdr_code.p_filesz == img_off->phdr_code.p_memsz );
  FD_TEST( img_off->phdr_ram .p_filesz <= img_off->phdr_ram .p_memsz );

  FD_TEST( fd_ulong_is_aligned( img_off->phdr_rom .p_offset, FD_SHMEM_NORMAL_PAGE_SZ ) );
  FD_TEST( fd_ulong_is_aligned( img_off->phdr_code.p_offset, FD_SHMEM_NORMAL_PAGE_SZ ) );
  FD_TEST( fd_ulong_is_aligned( img_off->phdr_ram .p_offset, FD_SHMEM_NORMAL_PAGE_SZ ) );

  FD_TEST( fd_ulong_is_aligned( img_off->phdr_rom .p_vaddr,  FD_SHMEM_NORMAL_PAGE_SZ ) );
  FD_TEST( fd_ulong_is_aligned( img_off->phdr_code.p_vaddr,  FD_SHMEM_NORMAL_PAGE_SZ ) );
  FD_TEST( fd_ulong_is_aligned( img_off->phdr_ram .p_vaddr,  FD_SHMEM_NORMAL_PAGE_SZ ) );

  FD_TEST( fd_ulong_is_aligned( img_off->phdr_rom .p_filesz, FD_SHMEM_NORMAL_PAGE_SZ ) );
  FD_TEST( fd_ulong_is_aligned( img_off->phdr_code.p_filesz, FD_SHMEM_NORMAL_PAGE_SZ ) );
  FD_TEST( fd_ulong_is_aligned( img_off->phdr_ram .p_filesz, FD_SHMEM_NORMAL_PAGE_SZ ) );

  FD_TEST( fd_ulong_is_aligned( img_off->phdr_rom .p_memsz,  FD_SHMEM_NORMAL_PAGE_SZ ) );
  FD_TEST( fd_ulong_is_aligned( img_off->phdr_code.p_memsz,  FD_SHMEM_NORMAL_PAGE_SZ ) );
  FD_TEST( fd_ulong_is_aligned( img_off->phdr_ram .p_memsz,  FD_SHMEM_NORMAL_PAGE_SZ ) );

  ulong off1;
  FD_TEST( !__builtin_uaddl_overflow( img_off->phdr_rom.p_offset,  img_off->phdr_rom .p_filesz, &off1 ) && off1<=bin_sz );
  FD_TEST( !__builtin_uaddl_overflow( img_off->phdr_code.p_offset, img_off->phdr_code.p_filesz, &off1 ) && off1<=bin_sz );
  FD_TEST( !__builtin_uaddl_overflow( img_off->phdr_ram.p_offset,  img_off->phdr_ram .p_filesz, &off1 ) && off1<=bin_sz );

  return img_off;
}

void
fdos_env_img_load( fdos_env_t *  env,
                   uchar const * bin_ro,
                   ulong         bin_sz ) {
  FD_TEST( fd_ulong_is_aligned( (ulong)bin_ro, FD_SHMEM_NORMAL_PAGE_SZ ) );

  fdos_kern_img_off_t img_off[1];
  FD_TEST( fdos_kern_img_off_load( img_off, bin_ro, bin_sz ) );

  env->rodata = (fdos_vmo_t) {
    .gpaddr = img_off->phdr_rom.p_offset + FDOS_GPADDR_KERN_IMG,
    .gvaddr = img_off->phdr_rom.p_vaddr,
    .haddr  = (ulong)bin_ro + img_off->phdr_rom.p_offset,
    .sz     = img_off->phdr_rom.p_filesz
  };
  FD_TEST( fd_ulong_is_aligned( env->rodata.gpaddr, FD_SHMEM_NORMAL_PAGE_SZ ) );

  env->text = (fdos_vmo_t) {
    .gpaddr = img_off->phdr_code.p_offset + FDOS_GPADDR_KERN_IMG,
    .gvaddr = img_off->phdr_code.p_vaddr,
    .haddr  = (ulong)bin_ro + img_off->phdr_code.p_offset,
    .sz     = img_off->phdr_code.p_filesz
  };
  FD_TEST( fd_ulong_is_aligned( env->text.gpaddr, FD_SHMEM_NORMAL_PAGE_SZ ) );
  env->entry_gvaddr = img_off->ehdr.e_entry;

  ulong data_gaddr = fd_wksp_alloc( env->wksp_kern_data, FD_SHMEM_NORMAL_PAGE_SZ, img_off->phdr_ram.p_memsz, 1UL );
  FD_TEST( data_gaddr );
  uchar * data = fd_wksp_laddr_fast( env->wksp_kern_data, data_gaddr );
  fd_memcpy( data, bin_ro + img_off->phdr_ram.p_offset, img_off->phdr_ram.p_filesz );
  fd_memset( data + img_off->phdr_ram.p_filesz, 0, img_off->phdr_ram.p_memsz - img_off->phdr_ram.p_filesz );
  env->data = (fdos_vmo_t) {
    .gpaddr = img_off->phdr_ram.p_offset + FDOS_GPADDR_KERN_IMG,
    .gvaddr = img_off->phdr_ram.p_vaddr,
    .haddr  = (ulong)data, /* writable */
    .sz     = img_off->phdr_ram.p_memsz
  };

  /* Reserve space for page tables */
  ulong pm_max   = 32UL;
  ulong pm_gaddr = fd_wksp_alloc( env->wksp_kern_heap, FD_X86_PM_SZ, pm_max*FD_X86_PM_SZ, 1UL );
  FD_TEST( pm_gaddr );
  fdos_vmm_alloc_t * vmm_alloc = fdos_vmm_alloc_init(
      env->vmm_alloc,
      fd_wksp_laddr_fast( env->wksp_kern_heap, pm_gaddr ),
      pm_max * FD_X86_PM_SZ,
      FDOS_GPADDR_KERN_HEAP + pm_gaddr
  );
  ulong * pml4 = fdos_vmm_alloc_pml4( vmm_alloc );

  /* Map kernel */
  ulong text_flags   = FD_X86_PT_US|FD_X86_PT_G;
  ulong rodata_flags = FD_X86_PT_US|FD_X86_PT_G|FD_X86_PT_XD;
  ulong data_flags   = FD_X86_PT_US|FD_X86_PT_G|FD_X86_PT_XD|FD_X86_PT_RW;
  fdos_vmm_map_range( pml4, env->text  .gvaddr, env->text  .gpaddr, env->text.sz,   text_flags,   vmm_alloc );
  fdos_vmm_map_range( pml4, env->rodata.gvaddr, env->rodata.gpaddr, env->rodata.sz, rodata_flags, vmm_alloc );
  fdos_vmm_map_range( pml4, env->data  .gvaddr, env->data  .gpaddr, env->data.sz,   data_flags,   vmm_alloc );
}
