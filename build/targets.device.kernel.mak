kernel_src = $(ion_device_kernel_src) $(liba_kernel_src) $(kandinsky_minimal_src)

KERNEL_LDFLAGS = -Lion/src/$(PLATFORM)/kernel/flash -Lion/src/$(PLATFORM)/kernel/flash/$(MODEL)
KERNEL_LDDEPS += ion/src/$(PLATFORM)/kernel/flash/kernel_shared.ld ion/src/$(PLATFORM)/kernel/flash/$(MODEL)/canary.ld ion/src/$(PLATFORM)/kernel/flash/$(MODEL)/prologue.ld
