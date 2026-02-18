#include "fd_x86_idt.h"

char const *
fd_x86_interrupt_cstr( uint num ) {
  switch( num ) {
  case FD_X86_INT_DE:
    return "#DE-Divide";
  case FD_X86_INT_DB:
    return "#DB-Debug";
  case FD_X86_INT_NMI:
    return "NMI";
  case FD_X86_INT_BP:
    return "#BP-Breakpoint";
  case FD_X86_INT_OF:
    return "#OF-Overflow";
  case FD_X86_INT_BR:
    return "#BR-Bound";
  case FD_X86_INT_UD:
    return "#UD-Opcode";
  case FD_X86_INT_NM:
    return "#NM-No Math";
  case FD_X86_INT_DF:
    return "#DF-Double Fault";
  case FD_X86_INT_TS:
    return "#TS-Invalid TSS";
  case FD_X86_INT_NP:
    return "#NP-Invalid Segment";
  case FD_X86_INT_SS:
    return "#SS-Stack Segfault";
  case FD_X86_INT_GP:
    return "#GP-General Protection Fault";
  case FD_X86_INT_PF:
    return "#PF-Page Fault";
  case FD_X86_INT_MF:
    return "#MF-Math Fault";
  case FD_X86_INT_AC:
    return "#AC-Alignment";
  case FD_X86_INT_MC:
    return "#MC-Machine Check";
  case FD_X86_INT_XM:
    return "#XM-SIMD Float";
  case FD_X86_INT_VE:
    return "#VE-Virtualization";
  case FD_X86_INT_CP:
    return "#CP-Control Protection";
  case FD_X86_INT_HV:
    return "#HV-Hypervisor Injection";
  case FD_X86_INT_VC:
    return "#VC-VMM Communication";
  case FD_X86_INT_SX:
    return "#SX-Security Exception";
  default:
    if( num>=0x20 ) return "IRQ";
    return "??";
  }
}
