#define _GNU_SOURCE /* fmemopen */
#include "fdos_vmm.h"
#include "x86/fd_x86_mmu.h"
#include "../util/fd_util.h"
#include <errno.h>
#include <stdio.h>

static uchar __attribute__((aligned(FD_X86_PM_SZ)))
pm_buf[ 8*FD_X86_PM_SZ ];

static void
assert_vmm_printf_eq( ulong const *            pml4,
                      fdos_vmm_alloc_t const * alloc,
                      char const *             expected ) {
  char buf[ 4096 ];
  FILE * file = fmemopen( buf, sizeof( buf ), "w" );
  FD_CRIT( file, "fmemopen failed" );
  int err = fdos_vmm_printf( pml4, file, alloc );
  if( FD_UNLIKELY( err ) ) FD_LOG_ERR(( "fdos_vmm_printf failed (%i-%s)", err, fd_io_strerror( err )) );
  long sz = ftell( file );
  if( FD_UNLIKELY( sz<0L ) ) FD_LOG_ERR(( "ftell failed (%i-%s)", errno, strerror( errno ) ));
  if( FD_UNLIKELY( 1!=fclose( file ) ) ) FD_LOG_ERR(( "fclose failed (%i-%s)", errno, strerror( errno ) ));
  if( FD_UNLIKELY( 0==strncmp( buf, expected, (ulong)sz ) ) ) {
    FD_LOG_ERR(( "fdos_vmm_printf returned unexpected result:\n%.*s", (int)sz, buf ));
  }
}

static void
test_page_4k( void ) {
  fdos_vmm_alloc_t alloc[1];
  FD_TEST( fdos_vmm_alloc_init( alloc, pm_buf, sizeof( pm_buf ), 0x1000000UL )==alloc );
  ulong * pml4 = fdos_vmm_alloc_pml4( alloc );
  fdos_vmm_map_range(
      pml4,
      0x200000UL,
      0x40000000UL,
      2*FD_X86_PML1E_RANGE,
      FD_X86_PT_RW | FD_X86_PT_US | FD_X86_PT_G | FD_X86_PT_XD,
      alloc
  );
  assert_vmm_printf_eq( pml4, alloc,
      ""
  );
}

static void
test_page_2m( void ) {

}

static void
test_page_1g( void ) {

}

int
main( int     argc,
      char ** argv ) {
  fd_boot( &argc, &argv );

  test_page_4k();
  test_page_2m();
  test_page_1g();

  FD_LOG_NOTICE(( "pass" ));
  fd_halt();
  return 0;
}
