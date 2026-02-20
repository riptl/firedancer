ifdef FD_FDOS_KERN
FDOS_KERN_OBJ := kern/fdos_kern kern/fdos_kern_log fdos_vmm
FDOS_KERN_OBJ_PATH := $(patsubst %,$(OBJDIR)/obj/fdos/%.o,$(FDOS_KERN_OBJ))
$(OBJDIR)/bin/fdos_kern.elf: src/fdos/kern/fdos_kern.ld $(FDOS_KERN_OBJ_PATH) $(OBJDIR)/lib/libfd_util.a
	mkdir -p $(dir $@) && \
    ld.lld \
	--no-undefined \
	--no-dynamic-linker \
	--static \
	-T src/fdos/kern/fdos_kern.ld \
	-o $@ \
	$(FDOS_KERN_OBJ_PATH) \
	$(OBJDIR)/lib/libfd_util.a \
	$(OPT)/cross/x86/lib/libc.a \
	$(OPT)/cross/x86/lib/libnosys.a
endif
