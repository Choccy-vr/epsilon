include ion/src/device/shared/usb/Makefile

ion_device_userland_src += $(addprefix ion/src/device/shared/boot/, \
  rt0.cpp \
)

ion_device_userland_src += $(addprefix ion/src/device/shared/drivers/, \
  base64.cpp:+n0100 \
  board_unprivileged.cpp \
  board_unprivileged_n0100.cpp:+n0100 \
  board_unprivileged_n0110.cpp:+n0110 \
  flash.cpp:+n0100 \
  flash_unprivileged.cpp:+n0110 \
  serial_number.cpp:+n0100 \
  trampoline.cpp:+n0110 \
  usb_unprivileged.cpp \
)

ion_device_userland_src += $(ion_device_dfu_src)
ion_device_userland_src += ion/src/device/shared/post_and_hardware_tests
