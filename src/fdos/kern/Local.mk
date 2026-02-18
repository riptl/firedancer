ifdef FD_FDOS_KERN
FDOS_KERN_OBJ := fdos_kern fdos_kern_log
FDOS_KERN_OBJ_PATH := $(patsubst %,$(OBJDIR)/obj/fdos/kern/%.o,$(FDOS_KERN_OBJ))
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
