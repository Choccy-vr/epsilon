# Define standard compilation rules

$(eval $(call rule_for, \
  AS, %.o, %.s, \
  $$(CC) $$(SFLAGS) -c $$< -o $$@, \
  global local \
))

$(eval $(call rule_for, \
  CC, %.o, %.c, \
  $$(CC) $$(CFLAGS) $$(SFLAGS) -c $$< -o $$@, \
  global local \
))

$(eval $(call rule_for, \
  CPP, %, %.inc, \
  $$(CPP) -P $$< $$@, \
  global \
))

$(eval $(call rule_for, \
  CXX, %.o, %.cpp, \
  $$(CXX) $$(CXXFLAGS) $$(SFLAGS) -c $$< -o $$@, \
  global local \
))

$(eval $(call rule_for, \
  DFUSE, %.dfu, , \
  $$(PYTHON) build/device/elf2dfu.py $$(DFUFLAGS) -i $$(filter-out $$(BUILD_DIR)/signer,$$^) -o $$@, \
  local \
))

$(eval $(call rule_for, \
  OBJCOPY, %.hex, %.elf, \
  $$(OBJCOPY) -O ihex $$< $$@, \
  local \
))

$(eval $(call rule_for, \
  OBJCOPY, %.bin, %.elf, \
  $$(OBJCOPY) -O binary $$< $$@, \
  local \
))

$(eval $(call rule_for, \
  OCC, %.o, %.m, \
  $$(CC) $$(CFLAGS) $$(SFLAGS) -c $$< -o $$@, \
  global \
))

$(eval $(call rule_for, \
  OCC, %.o, %.mm, \
  $$(CXX) $$(CXXFLAGS) $$(SFLAGS) -c $$< -o $$@, \
  global \
))

$(eval $(call rule_for, \
  WINDRES, %.o, %.rc, \
  $$(WINDRES) $$(WRFLAGS) $$< -O coff -o $$@, \
  global \
))

ifdef EXE
ifeq ($(OS),Windows_NT)
# Work around command-line length limit
# On Msys2 the max command line is 32 000 characters. Our standard LD command
# can be longer than that because we have quite a lot of object files. To work
# around this issue, we write the object list in a "target.objs" file, and tell
# the linker to read its arguments from this file.
$(eval $(call rule_for, \
  LD, %.$$(EXE), $$$$(LDSCRIPT), \
  echo $$(filter-out $$(LDSCRIPT),$$^) > $$@.objs && $$(LD) @$$@.objs $$(LDFLAGS) -o $$@ && rm $$@.objs, \
  global \
))
else
$(eval $(call rule_for, \
  LD, %.$$(EXE), $$$$(LDSCRIPT), \
  $$(LD) $$(filter-out $$(LDSCRIPT),$$^) $$(LDFLAGS) -o $$@, \
  global \
))
endif
endif
