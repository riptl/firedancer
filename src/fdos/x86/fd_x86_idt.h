#ifndef HEADER_fd_src_fdos_x86_fd_x86_idt_h
#define HEADER_fd_src_fdos_x86_fd_x86_idt_h

#include "../../util/fd_util_base.h"

struct __attribute__((packed)) fd_x86_idt_gate {
  ushort offset_low;
  ushort selector;
  uchar  ist;
  uchar  type_attr;
  ushort offset_mid;
  uint   offset_high;
  uint   reserved;
};

typedef struct fd_x86_idt_gate fd_x86_idt_gate_t;

#define FD_X86_INT_DE  0x00
#define FD_X86_INT_DB  0x01
#define FD_X86_INT_NMI 0x02
#define FD_X86_INT_BP  0x03
#define FD_X86_INT_OF  0x04
#define FD_X86_INT_BR  0x05
#define FD_X86_INT_UD  0x06
#define FD_X86_INT_NM  0x07
#define FD_X86_INT_DF  0x08
#define FD_X86_INT_TS  0x0a
#define FD_X86_INT_NP  0x0b
#define FD_X86_INT_SS  0x0c
#define FD_X86_INT_GP  0x0d
#define FD_X86_INT_PF  0x0e
#define FD_X86_INT_MF  0x10
#define FD_X86_INT_AC  0x11
#define FD_X86_INT_MC  0x12
#define FD_X86_INT_XM  0x13
#define FD_X86_INT_VE  0x14
#define FD_X86_INT_CP  0x15
#define FD_X86_INT_HV  0x1c
#define FD_X86_INT_VC  0x1d
#define FD_X86_INT_SX  0x1e

char const *
fd_x86_interrupt_cstr( uint num );

#endif /* HEADER_fd_src_fdos_x86_fd_x86_idt_h */
