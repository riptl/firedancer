$(call make-lib,fdos)
$(call add-objs,fdos_vmm,fdos)

ifdef FD_HAS_LINUX
ifneq ($(wildcard build/fdos/kern/x86_64/bin/fdos_kern.elf),)

$(call add-objs,host/fdos_env,fdos_host)
$(call add-objs,host/fdos_kern_img,fdos_host)
$(call add-objs,host/fdos_kvm,fdos_host)
$(call add-objs,host/fdos_kvm_init,fdos_host)
$(call add-objs,host/fdos_migrate,fdos_host)
ifdef FD_HAS_LIBLLVM
$(call add-objs,x86/fd_x86_disasm,fdos_host)
endif
$(call add-objs,x86/fd_x86_idt,fdos_host)
$(call make-bin,test_fdos,test_fdos,fdos_host fdos fd_util)
$(call make-unit-test,test_vmm,test_vmm,fdos fd_util)

$(OBJDIR)/obj/fdos/test_fdos.o: build/fdos/kern/x86_64/bin/fdos_kern.elf

endif
endif
