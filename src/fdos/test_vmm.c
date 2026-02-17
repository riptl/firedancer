#define _GNU_SOURCE /* fmemopen */
#include "fdos_vmm.h"
#include "x86/fd_x86_mmu.h"
#include "../util/fd_util.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

static uchar __attribute__((aligned(FD_X86_PM_SZ)))
pm_buf[ 16*FD_X86_PM_SZ ];

static void
assert_vmm_printf_eq( ulong const *            pml4,
                      fdos_vmm_alloc_t const * alloc,
                      char const *             expected ) {
  ulong  bufsz = 1UL<<20;
  char * buf   = malloc( bufsz );
  FD_TEST( buf );
  FILE * file = fmemopen( buf, bufsz, "w" );
  FD_CRIT( file, "fmemopen failed" );
  int err = fdos_vmm_printf( pml4, file, alloc );
  if( FD_UNLIKELY( err ) ) FD_LOG_ERR(( "fdos_vmm_printf failed (%i-%s)", err, fd_io_strerror( err )) );
  long sz = ftell( file );
  if( FD_UNLIKELY( sz<0L ) ) FD_LOG_ERR(( "ftell failed (%i-%s)", errno, strerror( errno ) ));
  if( FD_UNLIKELY( 0!=fclose( file ) ) ) FD_LOG_ERR(( "fclose failed (%i-%s)", errno, strerror( errno ) ));
  if( FD_UNLIKELY( 0!=strncmp( buf, expected, (ulong)sz ) ) ) {
    FD_LOG_ERR(( "fdos_vmm_printf returned unexpected result (%ld bytes):\n%.*s", sz, (int)sz, buf ));
  }
  free( buf );
}

static void
test_vmm_map( void ) {
  fdos_vmm_alloc_t alloc[1];
  FD_TEST( fdos_vmm_alloc_init( alloc, pm_buf, sizeof( pm_buf ), 0x1000000UL )==alloc );
  ulong * pml4 = fdos_vmm_alloc_pml4( alloc );
  fdos_vmm_map_range(
      pml4,
      0x8080606000UL,
      0x202000UL,
      FD_X86_PML1E_RANGE,
      FD_X86_PT_US | FD_X86_PT_G,
      alloc
  );
  assert_vmm_printf_eq( pml4, alloc,
      "PML4\n"
      "|               000000000000..008000000000  --\n"
      "+-+ PML3                                    RO U       \n"
      "  |             008000000000..008080000000  --\n"
      "  +-+ PML2                                  RO U       \n"
      "    |           008080000000..008080600000  --\n"
      "    +-+ PML1                                RO U       \n"
      "      |         008080600000..008080606000  --\n"
      "      +---- 4K  008080606000..008080607000  RO U G     000000202000..000000203000\n"
      "                008080607000..008080800000  --\n"
      "                008080800000..0080c0000000  --\n"
      "                0080c0000000..010000000000  --\n"
      "                010000000000.1000000000000  --\n" );
  fdos_vmm_map_range(
      pml4,
      0x8080604000UL,
      0x200000UL,
      2*FD_X86_PML1E_RANGE,
      FD_X86_PT_RW | FD_X86_PT_US | FD_X86_PT_G | FD_X86_PT_XD,
      alloc
  );
  assert_vmm_printf_eq( pml4, alloc,
      "PML4\n"
      "|               000000000000..008000000000  --\n"
      "+-+ PML3                                    RW U       \n"
      "  |             008000000000..008080000000  --\n"
      "  +-+ PML2                                  RW U       \n"
      "    |           008080000000..008080600000  --\n"
      "    +-+ PML1                                RW U       \n"
      "      |         008080600000..008080604000  --\n"
      "      +---- 4K  008080604000..008080605000  RW U G XD  000000200000..000000201000\n"
      "      +---- 4K  008080605000..008080606000  RW U G XD  000000201000..000000202000\n"
      "      +---- 4K  008080606000..008080607000  RO U G     000000202000..000000203000\n"
      "                008080607000..008080800000  --\n"
      "                008080800000..0080c0000000  --\n"
      "                0080c0000000..010000000000  --\n"
      "                010000000000.1000000000000  --\n" );
  fdos_vmm_map_range(
      pml4,
      0x100c0805000UL,
      0x300000UL,
      2*FD_X86_PML1E_RANGE,
      0,
      alloc
  );
  assert_vmm_printf_eq( pml4, alloc,
      "PML4\n"
      "|               000000000000..008000000000  --\n"
      "+-+ PML3                                    RW U       \n"
      "| |             008000000000..008080000000  --\n"
      "| +-+ PML2                                  RW U       \n"
      "|   |           008080000000..008080600000  --\n"
      "|   +-+ PML1                                RW U       \n"
      "|     |         008080600000..008080604000  --\n"
      "|     +---- 4K  008080604000..008080605000  RW U G XD  000000200000..000000201000\n"
      "|     +---- 4K  008080605000..008080606000  RW U G XD  000000201000..000000202000\n"
      "|     +---- 4K  008080606000..008080607000  RO U G     000000202000..000000203000\n"
      "|               008080607000..008080800000  --\n"
      "|               008080800000..0080c0000000  --\n"
      "|               0080c0000000..010000000000  --\n"
      "+-+ PML3                                    RO K       \n"
      "  |             010000000000..0100c0000000  --\n"
      "  +-+ PML2                                  RO K       \n"
      "    |           0100c0000000..0100c0800000  --\n"
      "    +-+ PML1                                RO K       \n"
      "      |         0100c0800000..0100c0805000  --\n"
      "      +---- 4K  0100c0805000..0100c0806000  RO K       000000300000..000000301000\n"
      "      +---- 4K  0100c0806000..0100c0807000  RO K       000000301000..000000302000\n"
      "                0100c0807000..0100c0a00000  --\n"
      "                0100c0a00000..010100000000  --\n"
      "                010100000000..018000000000  --\n"
      "                018000000000.1000000000000  --\n" );
  fdos_vmm_map_range(
      pml4,
      0x100c0807000UL,
      0x100000UL,
      3*FD_X86_PML3E_RANGE,
      FD_X86_PT_RW | FD_X86_PT_US | FD_X86_PT_XD,
      alloc
  );
}

int
main( int     argc,
      char ** argv ) {
  fd_boot( &argc, &argv );

  test_vmm_map();

  FD_LOG_NOTICE(( "pass" ));
  fd_halt();
  return 0;
}
