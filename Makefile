BSP_BASE ?= ../freedom-e-sdk/bsp
BOARD ?= freedom-e300-lofive
PROGRAM ?= riscbot
LINK_TARGET ?= flash
GDB_PORT ?= 3333

include $(BSP_BASE)/env/$(BOARD)/settings.mk

RISCV_GCC     ?= $(RISCV_PATH)/bin/riscv64-unknown-elf-gcc
RISCV_GXX     ?= $(RISCV_PATH)/bin/riscv64-unknown-elf-g++
RISCV_OBJDUMP ?= $(RISCV_PATH)/bin/riscv64-unknown-elf-objdump
RISCV_GDB     ?= $(RISCV_PATH)/bin/riscv64-unknown-elf-gdb
RISCV_AR      ?= $(RISCV_PATH)/bin/riscv64-unknown-elf-ar

RISCV_OPENOCD_PATH ?= $(openocd_prefix)
RISCV_OPENOCD ?= $(RISCV_OPENOCD_PATH)/bin/openocd

#############################################################
# This Section is for Software Compilation
#############################################################
PROGRAM_DIR = software/$(PROGRAM)
PROGRAM_ELF = software/$(PROGRAM)/$(PROGRAM)

.PHONY: software_clean
clean: software_clean
	$(MAKE) -C $(PROGRAM_DIR) CC=$(RISCV_GCC) RISCV_ARCH=$(RISCV_ARCH) RISCV_ABI=$(RISCV_ABI) AR=$(RISCV_AR) BSP_BASE=$(BSP_BASE) BOARD=$(BOARD) LINK_TARGET=$(LINK_TARGET) clean

.PHONY: software
software: software_clean
	$(MAKE) -C $(PROGRAM_DIR) CC=$(RISCV_GCC) RISCV_ARCH=$(RISCV_ARCH) RISCV_ABI=$(RISCV_ABI) AR=$(RISCV_AR) BSP_BASE=$(BSP_BASE) BOARD=$(BOARD) LINK_TARGET=$(LINK_TARGET)

dasm: software $(RISCV_OBJDUMP)
	$(RISCV_OBJDUMP) -D $(PROGRAM_ELF)

#############################################################
# This Section is for uploading a program to SPI Flash
#############################################################
OPENOCDCFG ?= $(BSP_BASE)/env/$(BOARD)/openocd.cfg
OPENOCDARGS += -f $(OPENOCDCFG)

GDB_UPLOAD_ARGS ?= --batch

GDB_UPLOAD_CMDS += -ex "set remotetimeout 240"
GDB_UPLOAD_CMDS += -ex "target extended-remote localhost:$(GDB_PORT)"
GDB_UPLOAD_CMDS += -ex "monitor reset halt"
GDB_UPLOAD_CMDS += -ex "monitor flash protect 0 64 last off"
GDB_UPLOAD_CMDS += -ex "load"
GDB_UPLOAD_CMDS += -ex "monitor resume"
GDB_UPLOAD_CMDS += -ex "monitor shutdown"
GDB_UPLOAD_CMDS += -ex "quit"

upload:
	$(RISCV_OPENOCD) $(OPENOCDARGS) & \
	$(RISCV_GDB) $(PROGRAM_DIR)/$(PROGRAM) $(GDB_UPLOAD_ARGS) $(GDB_UPLOAD_CMDS) && \
	echo "Successfully uploaded '$(PROGRAM)' to $(BOARD)."

#############################################################
# This Section is for launching the debugger
#############################################################

run_openocd:
	$(RISCV_OPENOCD) $(OPENOCDARGS)

GDBCMDS += -ex "set remotetimeout 240"
GDBCMDS += -ex "target extended-remote localhost:$(GDB_PORT)"

run_gdb:
	$(RISCV_GDB) $(PROGRAM_DIR)/$(PROGRAM) $(GDBARGS) $(GDBCMDS)
