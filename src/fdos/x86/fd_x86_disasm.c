#include "fd_x86_disasm.h"
#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Disassembler.h>
#include "../../util/fd_util.h"

ulong
fd_x86_disasm( uchar const * code,
               ulong         rem,
               char          str[ FD_X86_DISASM_MAX ],
               ulong         rip ) {

  static LLVMDisasmContextRef disasm_ctx;
  FD_THREAD_ONCE_BEGIN {
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllDisassemblers();

    disasm_ctx = LLVMCreateDisasm( "x86_64-unknown-linux-gnu", NULL, 0, NULL, NULL );
    FD_TEST( disasm_ctx );

    ulong options =
        LLVMDisassembler_Option_PrintImmHex|
        LLVMDisassembler_Option_AsmPrinterVariant;
#   ifdef LLVMDisassembler_Option_Color
    options |= LLVMDisassembler_Option_Color;
#   endif
    FD_TEST( 1==LLVMSetDisasmOptions( disasm_ctx, options ) );
  }
  FD_THREAD_ONCE_END;

  ulong cnt = LLVMDisasmInstruction( disasm_ctx, (uchar *)code, rem, rip, str, FD_X86_DISASM_MAX );
  if( FD_UNLIKELY( cnt==0 ) ) return 0UL;

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

  return cnt;
}
