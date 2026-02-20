#include "fdos_cpuid.h"
#include "../x86/fd_x86_cpuid.h"
#include "../../util/log/fd_log.h"

static uint const FDOS_X86_CPUID_01_0_ECX_REQUIRED =
    FD_X86_CPUID_01_ECX_SSE3  |  /* basic features */
    FD_X86_CPUID_01_ECX_SSSE3 |
    FD_X86_CPUID_01_ECX_SSE41 |
    FD_X86_CPUID_01_ECX_SSE42 |
    FD_X86_CPUID_01_ECX_POPCNT;

static uint const FDOS_X86_CPUID_01_0_EDX_REQUIRED =
    FD_X86_CPUID_01_EDX_TSC  |  /* fast clock */
    FD_X86_CPUID_01_EDX_PAE  |  /* paging */
    FD_X86_CPUID_01_EDX_PGE  |
    FD_X86_CPUID_01_EDX_CMOV |  /* basic features */
    FD_X86_CPUID_01_EDX_CLFL |  /* cache flush */
    FD_X86_CPUID_01_EDX_MMX  |  /* basic features */
    FD_X86_CPUID_01_EDX_SSE  |
    FD_X86_CPUID_01_EDX_SSE2;

static uint const FDOS_X86_CPUID_07_0_EBX_REQUIRED =
    0U;

void
fdos_cpuid_check_init( fdos_cpuid_check_t * check ) {
  memset( check, 0, sizeof(fdos_cpuid_check_t) );
}

static void
check_cpuid_features( ulong        path,
                      char const * reg,
                      uint         have,
                      uint         required ) {
  if( FD_LIKELY( ( have & required )==required ) ) return;
  uint leaf    = path >> 32;
  uint subleaf = path & UINT_MAX;
  uint missing = required & ~have;
  char missing_cstr[ 256 ];
  char * p = fd_cstr_init( missing_cstr );
  for( uint idx=0U; idx<32U; idx++ ) {
    if( !( missing & (1U<<idx) ) ) continue;
    if( p!=missing_cstr ) p = fd_cstr_append_char( p, ',' );
    p = fd_cstr_append_uint_as_text( p, ' ', 0, idx, fd_uint_base10_dig_cnt( idx ) );
  }
  fd_cstr_fini( p );
  FD_LOG_ERR(( "KVM vCPU is missing required features (CPUID %08x:%02x %s is missing bits %s)\nSee https://www.sandpile.org/x86/cpuid.htm",
               leaf, subleaf, reg, missing_cstr ));
}

void
fdos_cpuid_check_push( fdos_cpuid_check_t * check,
                       uint                 leaf,
                       uint                 subleaf,
                       uint                 eax,
                       uint                 ebx,
                       uint                 ecx,
                       uint                 edx ) {
  ulong path = ( (ulong)leaf << 32 ) | subleaf;
  (void)eax;
  switch( path ) {
# define CPUID_PATH( LEAF, SUBLEAF ) ( ( (ulong)(LEAF) << 32 ) | (ulong)(SUBLEAF) )
  case CPUID_PATH( 0x0001U, 0x00 ):
    check->cpuid_01_0 = 1;
    check_cpuid_features( path, "ecx", ecx, FDOS_X86_CPUID_01_0_ECX_REQUIRED );
    check_cpuid_features( path, "edx", edx, FDOS_X86_CPUID_01_0_EDX_REQUIRED );
    check->cpu_feat |=
        ( edx & FD_X86_CPUID_01_EDX_SSE ) ? FDOS_CPU_FEAT_REG_XMM : 0UL;
    check->cpu_feat |=
        ( ecx & FD_X86_CPUID_01_ECX_AVX ) ? FDOS_CPU_FEAT_REG_YMM : 0UL;
    break;
  case CPUID_PATH( 0x0007U, 0x00 ):
    check->cpuid_07_0 = 1;
    check_cpuid_features( path, "ebx", ebx, FDOS_X86_CPUID_07_0_EBX_REQUIRED );
    check->cpu_feat |=
        ( ebx & FD_X86_CPUID_07_0_EBX_AVX512F ) ? FDOS_CPU_FEAT_REG_ZMM : 0UL;
    break;
# undef CPUID_PATH
  }
}

void
fdos_cpuid_validate( fdos_cpuid_check_t const * check ) {
  if( FD_UNLIKELY( !check->cpuid_01_0 ) ) FD_LOG_ERR(( "CPUID 00000001:00 is missing" ));
  if( FD_UNLIKELY( !check->cpuid_07_0 ) ) FD_LOG_ERR(( "CPUID 00000007:00 is missing" ));

  ulong     cpu_feat = check->cpu_feat;
  int const feat_xmm = !!( cpu_feat & FDOS_CPU_FEAT_REG_XMM );
  int const feat_ymm = !!( cpu_feat & FDOS_CPU_FEAT_REG_YMM );
  int const feat_zmm = !!( cpu_feat & FDOS_CPU_FEAT_REG_ZMM );
  if( FD_UNLIKELY( !feat_xmm ) ) FD_LOG_ERR(( "vCPU has no xmm registers (CPUID detection broken?)" ));
  if( feat_ymm ) FD_LOG_INFO(( "vCPU has ymm registers (AVX)"    ));
  if( feat_zmm ) FD_LOG_INFO(( "vCPU has zmm registers (AVX512)" ));
  if( FD_UNLIKELY( feat_zmm && !feat_ymm ) ) FD_LOG_ERR(( "vCPU has zmm registers but no ymm registers (CPUID detection broken?)" ));
}
