#include "fdos_vmm.h"
#include "x86/fd_x86_mmu.h"
#include "../util/bits/fd_bits.h"
#include "../util/log/fd_log.h"

fdos_vmm_alloc_t *
fdos_vmm_alloc_init( fdos_vmm_alloc_t * alloc,
                     void *             buf,
                     ulong              sz,
                     ulong              gpaddr ) {
  alloc->next   = 0;
  alloc->max    = sz;
  alloc->gpaddr = gpaddr;
  alloc->haddr  = (ulong)buf;
  return alloc;
}

static inline ulong
fdos_vmm_alloc( fdos_vmm_alloc_t * a ) {
  if( FD_UNLIKELY( a->next >= a->max ) ) {
    FD_LOG_CRIT(( "cannot create page table, out of memory" ));
  }
  ulong p = a->next;
  a->next += FD_X86_PM_SZ;
  memset( (void *)( a->haddr+p ), 0, FD_X86_PM_SZ );
  return p;
}

static inline ulong *
fdos_pt_gpaddr_to_haddr( fdos_vmm_alloc_t const * a,
                         ulong                   gpaddr ) {
  return (ulong *)( a->haddr + ( gpaddr - a->gpaddr ) );                  
}

ulong *
fdos_vmm_alloc_pml4( fdos_vmm_alloc_t * alloc ) {
  ulong gpaddr = alloc->gpaddr + fdos_vmm_alloc( alloc );
  return fdos_pt_gpaddr_to_haddr( alloc, gpaddr );
}

static void
fdos_env_map_pml1( ulong * pml1,
                   ulong   vaddr,
                   ulong   paddr,
                   ulong   sz,
                   ulong   flags ) {
  ulong page_flags = flags &
      ( FD_X86_PT_RW | FD_X86_PT_US | FD_X86_PT_G | FD_X86_PT_XD );
  ulong vaddr1 = fd_ulong_min( vaddr+sz, fd_ulong_align_up( vaddr+1UL, FD_X86_PML2E_RANGE ) );
  while( vaddr<vaddr1 ) { /* each PML1E */
    ulong pml1e_idx = fd_ulong_extract( vaddr, 12, 20 );
    pml1[ pml1e_idx ] = paddr | FD_X86_PT_P | FD_X86_PT_PS | page_flags;
    vaddr += FD_X86_PML1E_RANGE;
    paddr += FD_X86_PML1E_RANGE;
  }
}

static void
fdos_env_map_pml2( ulong *            pml2,
                   ulong              vaddr,
                   ulong              paddr,
                   ulong              sz,
                   ulong              flags,
                   fdos_vmm_alloc_t * alloc ) {
  ulong page_flags = flags &
      ( FD_X86_PT_RW | FD_X86_PT_US | FD_X86_PT_G | FD_X86_PT_XD );
  ulong table_flags = flags &
      ( FD_X86_PT_RW | FD_X86_PT_US );
  ulong vaddr1 = fd_ulong_min( vaddr+sz, fd_ulong_align_up( vaddr+1UL, FD_X86_PML3E_RANGE ) );
  while( vaddr<vaddr1 ) { /* each PML2E */
    ulong pml2e_idx = fd_ulong_extract( vaddr, 21, 29 );
    if( !pml2[ pml2e_idx ] ) {
      if( !( vaddr & fd_ulong_mask_lsb( 21 ) ) &&
          vaddr1-vaddr >= FD_X86_PML2E_RANGE ) { /* 2 MiB huge page? */
        pml2[ pml2e_idx ] = paddr | FD_X86_PT_P | FD_X86_PT_PS | page_flags;
        goto next;
      }
      ulong off = fdos_vmm_alloc( alloc );
      pml2[ pml2e_idx ] = (alloc->gpaddr + off) | FD_X86_PT_P | table_flags;
    }
    pml2[ pml2e_idx ] |= table_flags;
    ulong * pml1 = fdos_pt_gpaddr_to_haddr( alloc, fd_x86_mmu_paddr( pml2[ pml2e_idx ] ) );
    fdos_env_map_pml1( pml1, vaddr, paddr, sz, flags );
  next:
    vaddr = fd_ulong_align_up( vaddr+1UL, FD_X86_PML2E_RANGE );
    paddr = fd_ulong_align_up( paddr+1UL, FD_X86_PML2E_RANGE );
  }
}

static void
fdos_env_map_pml3( ulong *            pml3,
                   ulong              vaddr,
                   ulong              paddr,
                   ulong              sz,
                   ulong              flags,
                   fdos_vmm_alloc_t * alloc ) {
  ulong vaddr1 = fd_ulong_min( vaddr+sz, fd_ulong_align_up( vaddr+1UL, FD_X86_PML4E_RANGE ) );
  ulong page_flags = flags &
      ( FD_X86_PT_RW | FD_X86_PT_US | FD_X86_PT_G | FD_X86_PT_XD );
  ulong table_flags = flags &
      ( FD_X86_PT_RW | FD_X86_PT_US );
  while( vaddr<vaddr1 ) { /* each PML3E */
    ulong pml3e_idx = fd_ulong_extract( vaddr, 30, 38 );
    if( !pml3[ pml3e_idx ] ) {
      if( !( vaddr & fd_ulong_mask_lsb( 30 ) ) &&
          vaddr1-vaddr >= FD_X86_PML3E_RANGE ) { /* huge page? */
        pml3[ pml3e_idx ] = paddr | FD_X86_PT_P | FD_X86_PT_PS | page_flags;
        goto next;
      }
      ulong off = fdos_vmm_alloc( alloc );
      pml3[ pml3e_idx ] = (alloc->gpaddr + off) | FD_X86_PT_P | table_flags;
    }
    pml3[ pml3e_idx ] |= table_flags;
    ulong * pml2 = fdos_pt_gpaddr_to_haddr( alloc, fd_x86_mmu_paddr( pml3[ pml3e_idx ] ) );
    fdos_env_map_pml2( pml2, vaddr, paddr, sz, flags, alloc );
  next:
    vaddr = fd_ulong_align_up( vaddr+1UL, FD_X86_PML3E_RANGE );
    paddr = fd_ulong_align_up( paddr+1UL, FD_X86_PML3E_RANGE );
  }
}

static void
fdos_env_map_pml4( ulong *            pml4,
                   ulong              vaddr,
                   ulong              paddr,
                   ulong              sz,
                   ulong              flags,
                   fdos_vmm_alloc_t * alloc ) {
  ulong vaddr1 = vaddr+sz;
  ulong table_flags = flags & ( FD_X86_PT_RW | FD_X86_PT_US );
  while( vaddr<vaddr1 ) { /* each PML4E */
    ulong pml4e_idx = fd_ulong_extract( vaddr, 39, 47 );
    if( !pml4[ pml4e_idx ] ) {
      ulong off = fdos_vmm_alloc( alloc );
      pml4[ pml4e_idx ] = (alloc->gpaddr + off) | FD_X86_PT_P | table_flags;
    }
    pml4[ pml4e_idx ] |= table_flags;
    ulong * pml3 = fdos_pt_gpaddr_to_haddr( alloc, fd_x86_mmu_paddr( pml4[ pml4e_idx ] ) );
    fdos_env_map_pml3( pml3, vaddr, paddr, sz, flags, alloc );
    vaddr = fd_ulong_align_up( vaddr+1UL, FD_X86_PML4E_RANGE );
    paddr = fd_ulong_align_up( paddr+1UL, FD_X86_PML4E_RANGE );
  }
}

void
fdos_vmm_map_range( ulong *            pml4,
                    ulong              vaddr,
                    ulong              paddr,
                    ulong              sz,
                    ulong              flags,
                    fdos_vmm_alloc_t * alloc ) {
  ulong       paddr0 = paddr;
  ulong const paddr1 = paddr+sz;
  ulong       vaddr0 = vaddr;
  ulong const vaddr1 = vaddr+sz;
  FD_LOG_INFO(( "Mapping vaddr [%#lx,%#lx) to paddr [%#lx,%#lx)", vaddr0, vaddr1, paddr0, paddr1 ));
  FD_CRIT( fd_ulong_is_aligned( paddr0, FD_X86_PML1E_RANGE ), "invalid argument" );
  FD_CRIT( fd_ulong_is_aligned( paddr1, FD_X86_PML1E_RANGE ), "invalid argument" );
  FD_CRIT( fd_ulong_is_aligned( vaddr0, FD_X86_PML1E_RANGE ), "invalid argument" );
  FD_CRIT( fd_ulong_is_aligned( vaddr1, FD_X86_PML1E_RANGE ), "invalid argument" );
  FD_CRIT( paddr0<=paddr1, "invalid argument" );
  FD_CRIT( vaddr0<=vaddr1, "invalid argument" );
  fdos_env_map_pml4( pml4, vaddr0, paddr0, sz, flags, alloc );
}

#if FD_HAS_HOSTED

#include <stdio.h>

static char *
append_flags( char * p,
              ulong  pme ) {
  int is_rw = !!( pme & FD_X86_PT_RW );
  int is_us = !!( pme & FD_X86_PT_US );
  int is_g  = !!( pme & FD_X86_PT_G  );
  int is_xd = !!( pme & FD_X86_PT_XD );
  p = fd_cstr_append_text( p, "  ", 2 );
  p = fd_cstr_append_text( p, is_rw?"RW":"RO", 2 );
  p = fd_cstr_append_char( p, ' ' );
  p = fd_cstr_append_text( p, is_us?"U":"K", 1 );
  p = fd_cstr_append_char( p, ' ' );
  p = fd_cstr_append_text( p, is_g?"G":" ", 1 );
  p = fd_cstr_append_char( p, ' ' );
  p = fd_cstr_append_text( p, is_xd?"XD":"  ", 2 );
  p = fd_cstr_append_text( p, "  ", 2 );
  return p;
}

static char *
append_ptr( char * p,
            ulong  ptr ) {
  return fd_cstr_append_ulong_as_hex( p, '0', ptr, 12 );
}

static char *
append_range( char * p,
              ulong  base,
              ulong  sz ) {
  ulong v0 = base;
  ulong v1 = base+sz;    
  p = append_ptr( p, v0 );
  p = fd_cstr_append_text( p, "..", 2 );
  p = append_ptr( p, v1 );
  return p;
}

static int
printf_gap( FILE * file,
            char   prefix[ 6 ],
            ulong  base,
            ulong  cnt,
            ulong  pml,
            int    tail ) {
  char line[128];
  ulong depth = 4UL-pml;
  char * p = fd_cstr_init( line );
  p = fd_cstr_append_text( p, prefix, depth*2UL );
  p = fd_cstr_append_char( p, tail?' ':'|' );
  p = fd_cstr_append_text( p, "               ", 15-(depth*2UL) );
  p = append_range( p, base, (cnt<<3)<<(pml*9) );
  p = fd_cstr_append_text( p, "  --\n", 5 );
  if( FD_UNLIKELY( 1!=fwrite( line, (ulong)( p-line ), 1, file ) ) ) {
    return ferror( file );
  }
  return 0;
}

static ulong
pml_last_idx( ulong const * pml ) {
  ulong i;
  for( i=511UL; i>0UL; i-- ) {
    if( pml[i] & FD_X86_PT_P ) break;
  }
  return i;
}

static ulong const *
pml_translate( ulong                    pme,
               fdos_vmm_alloc_t const * alloc ) {
  ulong gpaddr = fd_x86_mmu_paddr( pme );
  if( FD_UNLIKELY( gpaddr < alloc->gpaddr ) ) return NULL;
  ulong off = gpaddr - alloc->gpaddr;
  if( FD_UNLIKELY( off >= alloc->max ) ) return NULL;
  return (ulong const *)( alloc->haddr + off );
}

static int
printf_pml1( ulong const * pml1, 
             ulong         vaddr0,
             FILE *        file,
             char          prefix[ 6 ] ) {
  char  line[ 128 ];
  ulong found_idx = 0UL;
  for( ulong i=0UL; i<512UL; i++ ) {
    if( !( pml1[i] & FD_X86_PT_P ) ) continue;
    ulong vaddr = vaddr0 + (i<<12);
    if( i>found_idx ) {
      int err = printf_gap( file, prefix, vaddr0+(found_idx<<12), (i-found_idx), 1UL, 0 );
      if( FD_UNLIKELY( err ) ) return err;
    }
    found_idx = i+1UL;

    char * p = fd_cstr_init( line );
    p = fd_cstr_append_text( p, prefix, 6 );
    ulong gpaddr = fd_x86_mmu_paddr( pml1[ i ] );
    p = fd_cstr_append_cstr( p, "+---- 4K  " );
    p = fd_cstr_append_ulong_as_hex( p, '0', vaddr, 12 );
    p = fd_cstr_append_text( p, "..", 2 );
    p = fd_cstr_append_ulong_as_hex( p, '0', vaddr+FD_X86_PML1E_RANGE, 12 );
    p = append_flags( p, pml1[ i ] );
    p = fd_cstr_append_ulong_as_hex( p, '0', gpaddr, 12 );
    p = fd_cstr_append_text( p, "..", 2 );
    p = fd_cstr_append_ulong_as_hex( p, '0', gpaddr+FD_X86_PML1E_RANGE, 12 );
    p = fd_cstr_append_char( p, '\n' );
    fd_cstr_fini( p );
    if( FD_UNLIKELY( 1!=fwrite( line, (ulong)( p-line ), 1, file ) ) ) {
      return ferror( file );
    }
  }
  if( found_idx!=512UL ) {
    int err = printf_gap( file, prefix, vaddr0+(found_idx<<12), (512-found_idx), 1UL, 1 );
    if( FD_UNLIKELY( err ) ) return err;
  }
  return 0;
}

static int
printf_pml2( ulong const *            pml2, 
             ulong                    vaddr0,
             FILE *                   file,
             fdos_vmm_alloc_t const * alloc,
             char                     prefix[ 6 ] ) {
  ulong pml2_last_idx = pml_last_idx( pml2 );
  char  line[ 128 ];
  ulong found_idx = 0UL;
  for( ulong i=0UL; i<512UL; i++ ) {
    if( !( pml2[i] & FD_X86_PT_P ) ) continue;
    ulong vaddr = vaddr0 + (i<<21);
    if( i>found_idx ) {
      int err = printf_gap( file, prefix, vaddr0+(found_idx<<21), (i-found_idx), 2UL, 0 );
      if( FD_UNLIKELY( err ) ) return err;
    }
    found_idx = i+1UL;

    char * p = fd_cstr_init( line );
    p = fd_cstr_append_text( p, prefix, 4 );
    if( pml2[ i ] & FD_X86_PT_PS ) {
      ulong gpaddr = fd_x86_mmu_paddr( pml2[ i ] );
      p = fd_cstr_append_cstr( p, "+------ 2M  " );
      p = append_range( p, vaddr, FD_X86_PML2E_RANGE );
      p = append_flags( p, pml2[ i ] );
      p = append_range( p, gpaddr, FD_X86_PML2E_RANGE );
      p = fd_cstr_append_char( p, '\n' );
    } else {
      p = fd_cstr_append_cstr( p, "+-+ PML1                              " );
      p = append_flags( p, pml2[ i ] );
      p = fd_cstr_append_char( p, '\n' );
    }
    fd_cstr_fini( p );
    if( FD_UNLIKELY( 1!=fwrite( line, (ulong)( p-line ), 1, file ) ) ) {
      return ferror( file );
    }

    if( !( pml2[ i ] & FD_X86_PT_PS ) ) {
      prefix[ 4 ] = i!=pml2_last_idx ? '|' : ' ';
      ulong const * pml1 = pml_translate( pml2[i], alloc );
      if( FD_UNLIKELY( !pml1 ) ) continue;
      int err = printf_pml1( pml1, vaddr, file, prefix );
      if( err ) return err;
    }
  }
  if( found_idx!=512UL ) {
    int err = printf_gap( file, prefix, vaddr0+(found_idx<<21), (512-found_idx), 2UL, 1 );
    if( FD_UNLIKELY( err ) ) return err;
  }
  return 0;
}

static int
printf_pml3( ulong const *            pml3, 
             ulong                    vaddr0,
             FILE *                   file,
             fdos_vmm_alloc_t const * alloc,
             char                     prefix[ 6 ] ) {
  ulong pml3_last_idx = pml_last_idx( pml3 );
  char  line[ 128 ];
  ulong found_idx = 0UL;
  for( ulong i=0UL; i<512UL; i++ ) {
    if( !( pml3[i] & FD_X86_PT_P ) ) continue;
    ulong vaddr = vaddr0 + (i<<30);
    if( i>found_idx ) {
      int err = printf_gap( file, prefix, vaddr0+(found_idx<<30), (i-found_idx), 3UL, 0 );
      if( FD_UNLIKELY( err ) ) return err;
    }
    found_idx = i+1UL;
    
    char * p = fd_cstr_init( line );
    p = fd_cstr_append_text( p, prefix, 2 );
    if( pml3[ i ] & FD_X86_PT_PS ) {
      ulong gpaddr = fd_x86_mmu_paddr( pml3[ i ] );
      p = fd_cstr_append_cstr( p, "+-------- 1G  " );
      p = append_range( p, vaddr, FD_X86_PML3E_RANGE );
      p = append_flags( p, pml3[ i ] );
      p = append_range( p, gpaddr, FD_X86_PML3E_RANGE );
      p = fd_cstr_append_char( p, '\n' );
    } else {
      p = fd_cstr_append_cstr( p, "+-+ PML2                                " );
      p = append_flags( p, pml3[ i ] );
      p = fd_cstr_append_char( p, '\n' );
    }
    fd_cstr_fini( p );
    if( FD_UNLIKELY( 1!=fwrite( line, (ulong)( p-line ), 1, file ) ) ) {
      return ferror( file );
    }

    if( !( pml3[ i ] & FD_X86_PT_PS ) ) {
      prefix[ 2 ] = i!=pml3_last_idx ? '|' : ' ';
      ulong const * pml2 = pml_translate( pml3[i], alloc );
      if( FD_UNLIKELY( !pml2 ) ) continue;
      int err = printf_pml2( pml2, vaddr, file, alloc, prefix );
      if( err ) return err;
    }
  }
  if( found_idx!=512UL ) {
    int err = printf_gap( file, prefix, vaddr0+(found_idx<<30), (512-found_idx), 3UL, 1 );
    if( FD_UNLIKELY( err ) ) return err;
  }
  return 0;
}

static int
printf_pml4( ulong const *            pml4, 
             FILE *                   file,
             fdos_vmm_alloc_t const * alloc ) {
  char prefix[ 6 ];
  memset( prefix, ' ', 6 );
  if( FD_UNLIKELY( fputs( "PML4\n", file )<0 ) ) {
    return ferror( file );
  }
  ulong pml4_last_idx = pml_last_idx( pml4 );
  char  line[ 128 ];
  ulong found_idx = 0UL;
  for( ulong i=0UL; i<512UL; i++ ) {
    if( !( pml4[i] & FD_X86_PT_P ) ) continue;
    ulong vaddr = i<<39;
    if( i>found_idx ) {
      int err = printf_gap( file, prefix, found_idx<<39, (i-found_idx), 4UL, 0 );
      if( FD_UNLIKELY( err ) ) return err;
    }
    found_idx = i+1UL;
    
    char * p = fd_cstr_init( line );
    p = fd_cstr_append_cstr( p, "+-+ PML3                                  " );
    p = append_flags( p, pml4[ i ] );
    p = fd_cstr_append_char( p, '\n' );
    fd_cstr_fini( p );
    if( FD_UNLIKELY( 1!=fwrite( line, (ulong)( p-line ), 1, file ) ) ) {
      return ferror( file );
    }

    prefix[ 0 ] = i!=pml4_last_idx ? '|' : ' ';
    ulong const * pml3 = pml_translate( pml4[i], alloc );
    if( FD_UNLIKELY( !pml3 ) ) continue;
    int err = printf_pml3( pml3, vaddr, file, alloc, prefix );
    if( err ) return err;
  }
  if( found_idx!=512UL ) {
    int err = printf_gap( file, prefix, found_idx<<39, (512-found_idx), 4UL, 1 );
    if( FD_UNLIKELY( err ) ) return err;
  }
  return 0;
}

int
fdos_vmm_printf( ulong const *            pml4,
                 void *                   file_,
                 fdos_vmm_alloc_t const * alloc ) {
  FILE * file = (FILE *)file_;
  int err = printf_pml4( pml4, file, alloc );
  if( err ) return err;
  return 0;
}

#endif /* FD_HAS_HOSTED */
