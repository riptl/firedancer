#ifndef HEADER_fd_src_fdos_fdos_vmm_h
#define HEADER_fd_src_fdos_fdos_vmm_h

#include "../util/fd_util_base.h"

/* fdos_vmm_alloc_t is a bump allocator for page tables.
   This allocator is used to quickly construct sub-trees of the
   page table that can be freed in O(1) time. */

struct fdos_vmm_alloc {
  ulong next;   /* offset to next free page map */
  ulong max;    /* arena size */
  ulong gpaddr; /* guest physical base address */
  ulong haddr;  /* host base address */
};

typedef struct fdos_vmm_alloc fdos_vmm_alloc_t;

/* fdos_vmm_alloc_init creates a bump allocator for page tables.
   [buf,buf+sz) is the space available to the allocator, which
   must be mapped to guest physical memory.
   buf and sz must be aligned to FD_X86_PM_SZ.
   gpaddr is the guest physical address corresponding to buf. 
   Aborts with FD_LOG_CRIT if parameters are invalid. */

fdos_vmm_alloc_t *
fdos_vmm_alloc_init( fdos_vmm_alloc_t * alloc,
                     void *             buf,
                     ulong              sz,
                     ulong              gpaddr );

ulong *
fdos_vmm_alloc_pml4( fdos_vmm_alloc_t * alloc );

/* fd_vmm_map_range adds maps [paddr,paddr+sz) to [vaddr,vaddr+sz).
   pml4 points to the root page table.
   flags is a combination of the following flags:
   - FD_X86_PT_RW: allow write access
   - FD_X86_PT_US: allow userspace access
   - FD_X86_PT_G:  global mapping (keep in TLB when invalidating PCID)
   - FD_X86_PT_XD: execute disable
   New page tables are allocated from 'alloc' if necessary. */

void
fdos_vmm_map_range( ulong *            pml4,
                    ulong              paddr,
                    ulong              vaddr,
                    ulong              sz,
                    ulong              flags,
                    fdos_vmm_alloc_t * alloc );

#if FD_HAS_HOSTED

/* fdos_vmm_printf dumps the page table to a file.
   Lines are written to 'file', which is of 'FILE *' type.
   
   Assumes that all page maps are part of the same guest physical segment,
   which starts at base_gpaddr in guest physical address space, and base_haddr in
   host address space.
  
   An example page table looks as follows:
   
     PML4            000000000000
     ├─┐ PML3        000000000000                RW K
     │ ├──────── 1G  000000000000..000040000000  RW K G XD  10000000..50000000
     │ └─┐ PML2      000040000000
     │   ├────── 2M  000040000000..000040200000  RO K G     c0000000..c0200000
     │   └─┐ PML1    000040200000
     │     ├──── 4K  000040200000..000040201000  RO U       d0000000..d0001000
     │     └──── 4K  000040201000..000040202000  RW U   XD  d0001000..d0002000
     │               000040202000  --
     └─┐ PML3        008000000000
       │             008000000000..008080000000
       └──────── 1G  008080000000..0080c0000000  RW U   XD  50000000..90000000
  
   The output is unconditionally printed in colors.
   {Mapped,unmapped} ranges are printed {bright,dim}.
   
   Returns 0 on success and errno/ferror() on write error. */

int
fdos_vmm_printf( ulong const *            pml4,
                 void *                   file,
                 fdos_vmm_alloc_t const * alloc );

#endif /* FD_HAS_HOSTED*/

#endif /* HEADER_fd_src_fdos_fdos_vmm_h */
