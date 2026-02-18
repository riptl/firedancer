#include "fd_x86_disasm.h"
#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Disassembler.h>

char *
fd_x86_disasm( fdos_vmm_alloc_t const * vmm,
               fdos_phys_t const        phys[ FDOS_PIDX_MAX ],
               char                     str[ FD_X86_DISASM_MAX ],
               ulong                    rip ) {
  ulong gpaddr = fdos_gvaddr_to_gpaddr( rip, 1UL, vmm );
  ulong phys_idx;
  for( phys_idx=0UL; phys_idx<FDOS_PIDX_MAX; phys_idx++ ) {
    if( !!( gpaddr>=phys[ phys_idx ].gpaddr0 ) &
        !!( gpaddr< phys[ phys_idx ].gpaddr1 ) ) {
      break;
    }
  }
  if( phys_idx==FDOS_PIDX_MAX ) return NULL;

  ulong   off  = gpaddr - phys[ phys_idx ].gpaddr0;
  uchar * code = (uchar *)phys[ phys_idx ].haddr + off;
  ulong   rem  = phys[ phys_idx ].gpaddr1 - gpaddr;

  static LLVMDisasmContextRef disasm_ctx;
  FD_THREAD_ONCE_BEGIN {
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllDisassemblers();

    disasm_ctx = LLVMCreateDisasm( "x86_64-unknown-linux-gnu", NULL, 0, NULL, NULL );
    FD_TEST( disasm_ctx );

    FD_TEST( 1==LLVMSetDisasmOptions( disasm_ctx,
        LLVMDisassembler_Option_PrintImmHex|
        LLVMDisassembler_Option_AsmPrinterVariant ) );
  }
  FD_THREAD_ONCE_END;

  ulong cnt = LLVMDisasmInstruction( disasm_ctx, code, rem, rip, str, FD_X86_DISASM_MAX );
  if( FD_UNLIKELY( cnt==0 ) ) return NULL;

  /* Count number of chars excluding ANSI control chars.
     Also, replace tabs with spaces. */
  ulong visible_len = 0;
  ulong i           = 0;
  for( i=0; str[i]; i++ ) {
    if( str[i]=='\x1b' ) {
      /* Skip ANSI escape sequence */
      i++;
      while( str[i] && str[i]!='m' ) i++;
      if( !str[i] ) break;
      continue;
    }
    if( str[i]=='\t' ) {
      str[i] = ' ';
    }
    visible_len++;
  }

  /* Pad with spaces to ensure fixed width */
  for( ; visible_len<40 && i<FD_X86_DISASM_MAX-1; i++, visible_len++ ) {
    str[i] = ' ';
  }
  str[i] = '\0';

  return str;
}
