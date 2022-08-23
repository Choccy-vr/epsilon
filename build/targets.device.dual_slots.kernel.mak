include build/targets.$(PLATFORM).kernel.mak

HANDY_TARGETS += kernel.A kernel.B

# stack protector
SFLAGS += -fstack-protector-strong

kernel_obj = $(call flavored_object_for,$(kernel_src),$(MODEL) $(THIRD_PARTY_FLAVOR))
ifeq ($(EMBED_EXTRA_DATA),1)
kernel_obj += $(BUILD_DIR)/bootloader.o $(BUILD_DIR)/trampoline.o
endif

$(BUILD_DIR)/kernel.A.$(EXE): $(kernel_obj)
$(BUILD_DIR)/kernel.A.$(EXE): LDSCRIPT = ion/src/$(PLATFORM)/kernel/flash/kernel_A.ld

$(BUILD_DIR)/kernel.B.$(EXE): $(kernel_obj)
$(BUILD_DIR)/kernel.B.$(EXE): LDSCRIPT = ion/src/$(PLATFORM)/kernel/flash/kernel_B.ld

$(BUILD_DIR)/kernel.%.$(EXE): LDFLAGS += $(KERNEL_LDFLAGS)
$(BUILD_DIR)/kernel.%.$(EXE): LDDEPS += $(KERNEL_LDDEPS) $(LDSCRIPT)
