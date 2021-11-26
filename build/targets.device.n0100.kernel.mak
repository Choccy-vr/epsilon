include build/targets.$(PLATFORM).kernel.mak

HANDY_TARGETS += kernel

kernel_src += $(liba_internal_flash_src)
kernel_obj = $(call flavored_object_for,$(kernel_src),)
$(BUILD_DIR)/kernel.$(EXE): $(kernel_obj)
$(BUILD_DIR)/kernel.$(EXE): LDFLAGS += $(KERNEL_LDFLAGS)
$(BUILD_DIR)/kernel.$(EXE): LDSCRIPT = $(KERNEL_LDSCRIPT)
$(BUILD_DIR)/kernel.$(EXE): LDDEPS += $(KERNEL_LDDEPS)
