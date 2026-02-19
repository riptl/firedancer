#include "fdos_migrate.h"
#include "../user/fdos_user.h"
#include "../x86/fd_x86_mmu.h"
#include "fdos_env.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

static ulong
copy_range( fdos_phys_t *      phys,
            fdos_vmm_alloc_t * vmm,
            ulong *            pml4,
            ulong              off0,
            ulong              haddr0,
            ulong              sz,
            ulong              flags ) {

  ulong haddr1  = haddr0 + sz;
  ulong phys_sz = phys->gpaddr1 - phys->gpaddr0;

  /* Don't migrate guest physical memory itself */
  if( FD_UNLIKELY( ( haddr0 >= phys->haddr ) &
                   ( haddr1 <= phys->haddr + phys_sz ) ) ) {
    return off0;
  }

  /* Don't migrate anything in low 4 GiB space */
  if( FD_UNLIKELY( haddr0 <= UINT_MAX ) ) {
    return off0;
  }

  /* Don't migrate anything in kernel half */
  if( FD_UNLIKELY( haddr0 >= 0x800000000000UL ) ) {
    return off0;
  }

  /* Copy range into guest physical memory */
  ulong off1 = off0 + sz;
  if( FD_UNLIKELY( off1 > phys_sz ) ) {
    FD_LOG_ERR(( "cannot migrate host range vaddr=[%#lx,%#lx) sz=%4lu KiB: userland VMO max size of %lu bytes exceeded",
                 haddr0, haddr0+sz, sz>>10, phys_sz ));
  }
  fd_memcpy( (uchar *)phys->haddr + off0, (void const *)haddr0, sz );

  /* Map guest physical memory to guest virtual memory
     (Host and guest virtual addresses are identical) */
  ulong page_flags = flags | FD_X86_PT_G | FD_X86_PT_US;
  fdos_vmm_map_range( pml4, haddr0, phys->gpaddr0 + off0, sz, page_flags, vmm );

  return fd_ulong_align_up( off1, FD_SHMEM_NORMAL_PAGE_SZ );
}

static void
copy_vmm( fdos_phys_t *      phys,
          fdos_vmm_alloc_t * alloc ) {
  ulong * pml4 = (ulong *)alloc->haddr;

  FILE * file = fopen( "/proc/self/maps", "r" );
  if( FD_UNLIKELY( !file ) ) {
    FD_LOG_ERR(( "fopen(/proc/self/maps) failed (%i-%s)", errno, fd_io_strerror( errno ) ));
  }

  ulong vmo_off = FD_SHMEM_NORMAL_PAGE_SZ;
  for(;;) {
    char line[ 1024 ];
    char * p = fgets( line, sizeof(line), file );
    if( !p ) break;

    if( strstr( p, "[vvar"      ) ) continue;
    if( strstr( p, "[vdso]"     ) ) continue;
    if( strstr( p, "[vsyscall]" ) ) continue;
    if( strstr( p, ".so" ) ) {
      /* permitted libraries */
      if( strstr( p, "libc.so"   ) ) goto permit;
      if( strstr( p, "libm.so"   ) ) goto permit;
      if( strstr( p, "libgcc.so" ) ) goto permit;
      continue;
    permit:;
    }

    ulong m0;
    ulong m1;
    char  perms[5];
    int   len = 0;
    int r = sscanf( p, "%lx-%lx %4s %*s %*s %*lu%n", &m0, &m1, perms, &len );
    if( FD_UNLIKELY( r!=3 || len==0 ) ) continue;

    // char * path = p + len;
    // while( *path && *path==' ' ) path++;
    // char * path_end = path;
    // while( *path_end && *path_end!=' ' && *path_end!='\n' ) path_end++;
    // *path_end = '\0';

    /* Don't copy Firedancer workspaces */
    fd_shmem_join_info_t info[1];
    if( 0==fd_shmem_join_query_by_addr( (void const *)m0, m1-m0, info ) ) continue;

    int is_read  = perms[ 0 ]=='r';
    int is_write = perms[ 1 ]=='w';
    int is_exec  = perms[ 2 ]=='x';
    if( !is_read ) continue;

    ulong page_flags =
        ( is_write ? FD_X86_PT_RW : 0UL ) |
        ( is_exec  ? 0UL : FD_X86_PT_XD );
    vmo_off = copy_range( phys, alloc, pml4, vmo_off, m0, m1-m0, page_flags );
  }

  fclose( file );
}

static void
patch_trampoline( fdos_env_t * env,
                  ulong        gvaddr,
                  ulong        new_func ) {
  uchar patch[] = {
    0x48, 0xb8, /* movabs rax, imm64 */
    0,0,0,0,0,0,0,0, /* imm64 placeholder */
    0xff, 0xe0  /* jmp rax */
  };
  FD_STORE( ulong, patch+2, new_func );

  ulong gpaddr = fdos_gvaddr_to_gpaddr( gvaddr, sizeof(patch), env->vmm_alloc );
  FD_TEST( gpaddr );
  uchar * haddr = fdos_gpaddr_to_haddr( gpaddr, sizeof(patch), env->phys );
  FD_TEST( haddr );
  fd_memcpy( haddr, patch, sizeof(patch) );
}

void
fdos_migrate_self( fdos_env_t * env ) {
  copy_vmm( &env->phys[ FDOS_PIDX_USER_MEM ], env->vmm_alloc );
  patch_trampoline( env, (ulong)write,         (ulong)fdos_user_write         );
  patch_trampoline( env, (ulong)clock_gettime, (ulong)fdos_user_clock_gettime );
}
