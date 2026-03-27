ifdef FD_FDOS_KERN

$(call make-lib,fdos_kern)
$(call add-objs,fdos_kern_main,fdos_kern)
$(call add-objs,fdos_kern_log,fdos_kern)
$(call add-objs,fdos_kern_ctx_idt fdos_kern_ctx_fred,fdos_kern)

FDOS_KERN_LIBS:=fdos_kern fdos fd_util
FDOS_KERN_LIB_PATHS:=$(patsubst %,$(OBJDIR)/lib/lib%.a,$(FDOS_KERN_LIBS))

$(OBJDIR)/bin/fdos_kern.elf: src/fdos/kern/fdos_kern.ld $(FDOS_KERN_LIB_PATHS)
	mkdir -p $(dir $@) && \
    ld.lld \
	--no-undefined \
	--no-dynamic-linker \
	--static \
	-T src/fdos/kern/fdos_kern.ld \
	-u fdos_kern_entry_idt \
	-u fdos_kern_entry_fred \
	-o $@ \
	$(FDOS_KERN_LIB_PATHS) \
	$(OPT)/cross/x86/lib/libc.a \
	$(OPT)/cross/x86/lib/libnosys.a

endif
