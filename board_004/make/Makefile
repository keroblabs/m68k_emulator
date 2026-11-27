##############################################################################################
#
# Rules to build console
#
##############################################################################################

##############################################################################################
# Start of main variables
#

DEFAULT_DRV_PATH = $(DEFAULT_PATH)/Boards/Common
APP_PATH = $(DEFAULT_PATH)/Applications/$(PROJECT)

GCCPATH = /Users/marciocherobim/Tools/gcc-4_7-2013q3/arm-none-eabi
LIBPATH = /home/marcio/Tools/m68k-elf-gcc/m68k-toolchain/lib/gcc/m68k-elf/6.3.0
TRGT    = /home/marcio/Tools/m68k-elf-gcc/m68k-toolchain/bin/m68k-elf-
LD		= $(TRGT)gcc
CC      = $(TRGT)gcc
CP      = $(TRGT)objcopy
AS      = $(TRGT)gcc -c -x assembler-with-cpp
SIZE    = $(TRGT)size
OBJCOPY = $(TRGT)objcopy
OBJDUMP = $(TRGT)objdump
BIN     = $(CP) -O ihex
MAKE    = make
CP      = cp
PYTHON  = python3

# Define project name here
PROJECT = m68k_rom
LDDEF = m68k.ld

#
# End of main variables
##############################################################################################

##############################################################################################
# Start of define section
#

# List all default C defines here, like -D_DEBUG=1
DEFS +=

# Physical external data bus used for byte-wide memory-mapped peripherals.
# 8 = MC68008 style; 16 = MC68000/MC68010 style with MC68681 on D0-D7/LDS.
M68K_DATA_BUS_WIDTH ?= 8
DEFS += -DM68K_DATA_BUS_WIDTH=$(M68K_DATA_BUS_WIDTH)

# Build with USE_V9958=1 for the Yamaha V9958 four-port video interface.
# The TMS9918 remains the default for existing Board 004 binaries.
USE_V9958 ?= 0
ifeq ($(USE_V9958),1)
ifneq ($(M68K_DATA_BUS_WIDTH),8)
$(error V9958 Board 004 currently requires an 8-bit data bus)
endif
DEFS += -DUSE_V9958=1
endif

# Keyboard model: 0 = UART keypad; 1 = YM2149/MC68681 scanned key matrix.
USE_KEYBOARD_MATRIX ?= 1
ifeq ($(USE_KEYBOARD_MATRIX),1)
DEFS += -DUSE_KEYBOARD_MATRIX=1
endif

# Key-matrix debounce: 1 = two-scan debounce (default); 0 = accept each scan
# immediately for the lowest input latency.
KB_USE_DEBOUNCE ?= 0
DEFS += -DKB_USE_DEBOUNCE=$(KB_USE_DEBOUNCE)

# List all default ASM defines here, like -D_DEBUG=1
ADEFS +=

#
# End of define section
##############################################################################################

##############################################################################################
# Start of include section
#

# List all default directories to look for include files here
CINCDIR  =
CINCDIR += ../
CINCDIR += ../boot
CINCDIR += ../system
CINCDIR += ../hardware
CINCDIR += ../framework
CINCDIR += ../source
CINCDIR += ../cli
CINCDIR += ../graphics
CINCDIR += ../vpd9918
CINCDIR += ../vdp9958

# End of include section
##############################################################################################

##############################################################################################
# Start of source section
#

SRC  =
SRC += ../boot/irq_handlers.c
SRC += ../boot/start.c
SRC += ../boot/syscalls.c

SRC += ../hardware/m68681_driver.c
SRC += ../hardware/uart_driver.c
SRC += ../hardware/timer_driver.c
SRC += ../hardware/ym2149_driver.c
SRC += ../hardware/keyboard_matrix_driver.c

SRC += ../system/debug.c
SRC += ../system/vsnprintk.c

SRC += ../framework/buffer.c
SRC += ../framework/timer.c
SRC += ../framework/vt100_input.c

SRC += ../cli/cli.c

SRC += ../vpd9918/vdp9918_register.c
SRC += ../vpd9918/vdp9918_driver.c
SRC += ../vpd9918/vdp9918_vt100.c
ifeq ($(USE_V9958),1)
SRC += ../vdp9958/vdp9958_driver.c
endif

SRC += ../source/main.c
SRC += ../source/cli_input.c
SRC += ../source/cli_ctrl.c

# List ASM source files here
ASRC  =
ASRC += ../boot/irq_boot.s
ASRC += ../boot/irq_vectors.s

#
# End of source section
##############################################################################################

# link information
LDSCRIPT = $(PROJECT).elf.ld

# Define optimisation level here
OPT = -Os
#OPT = -Os -gstabs

# Define debug level
##  -gdwarf-2
DEBUG = -g
#  -gstabs

# CPU flags
#MCFLAGS = -mcpu=$(MCU) $(THUMB)

# preprocessor flags
ADEFS += $(CPU_ADEFS)
DEFS  += $(CPU_DEFS)
DEFS  += $(OS_DEFS)

#include directories
INCDIR  = $(patsubst %,-I %,$(CINCDIR))
INCDIR += $(patsubst %,-I %,$(OS_CINCDIR))
INCDIR += $(patsubst %,-I %,$(CPU_CINCDIR))

#object and output files
ALLSRC  = $(ASRC) $(SRC) $(OS_ASRC) $(OS_SRC) $(CPU_ASRC) $(CPU_SRC)
OBJS    = $(ASRC:.s=.o) $(SRC:.c=.o) $(OS_ASRC:.s=.o) $(OS_SRC:.c=.o) $(CPU_ASRC:.s=.o) $(CPU_SRC:.c=.o)
#LIBS    = $(patsubst %,-L %,$(LIBDIR)) $(DLIBS) $(ULIBS)
OUTDIR  = ./

#
# C/C++ compiler flags
#
# -fno-rtti

#CFLAGS = -c -fsigned-char -mlittle-endian
CFLAGS = -c -fsigned-char
CFLAGS += -m68000
#CFLAGS += -ffunction-sections
#CFLAGS += -fno-exceptions
CFLAGS += $(MCFLAGS)
CFLAGS += $(DEFS)
CFLAGS += $(INCDIR) -I.
CFLAGS += $(OPT)
CFLAGS += $(DEBUG)

#
# ASM compiler flags
#

ASMFLAGS = -c -x assembler-with-cpp
ASMFLAGS += $(MCFLAGS)
ASMFLAGS += $(ADEFS)
ASMFLAGS += $(INCDIR)

#
# linker flags
#

LDFLAGS =
LDFLAGS += -Wl,-Map -Xlinker ./$(PROJECT).map
LDFLAGS += -Wl,--cref,--gc-sections
LDFLAGS += -m68000
#LDFLAGS += -nostdlib
#LDFLAGS += -Wl,-nodefaultlibs
LDFLAGS += -static
LDFLAGS += -nostartfiles
#LDFLAGS += -nostartfiles
#LDFLAGS += -nodefaultlibs
#LDFLAGS += -lc
#LDFLAGS += -lm
#LDFLAGS += -lrdimon
#LDFLAGS += -lstdc++
#LDFLAGS += -L$(GCCPATH)/lib
#LDFLAGS += -LC:/Tools/GCC_432/lib/ARM
#LDFLAGS += -L$(LIBDIR)
#LDFLAGS += -Wl,-T -Xlinker $(LDDEF)
LDFLAGS += -T $(LDDEF)
LDFLAGS += -L$(LIBPATH)
LDFLAGS += -lgcc -lc -lm
LDFLAGS += $(CPU_LD)
LDFLAGS += $(MCFLAGS)
LDFLAGS += $(OBJS)
#LDFLAGS += $(GCCPATH)/lib/ARM/std_sbrk_thumb.lib
#LDFLAGS += $(GCCPATH)/lib/ARM/smallprintf_thumb.a
#LDFLAGS += $(patsubst %,-L%,$(EXTRA_LIBDIRS))
#LDFLAGS += $(patsubst %,-l%,$(EXTRA_LIBS))


#
# makefile rules
#

all: DO_FLASH
clean: DO_CLEAN
rebuild: DO_REBUILD
flash: DO_DOWNLOAD

#
# internal makefile rules
#

DO_FLASH: show_start run_script $(OBJS) $(PROJECT).elf $(PROJECT).hex $(PROJECT).bin $(PROJECT).dis show_finished
DO_REBUILD: DO_CLEAN DO_FLASH
DO_CLEAN: exec_clean
DO_DOWNLOAD: $(PROJECT).fdn

#
# internal rules
#

%o : %c
	@echo ""
	@echo "C"
	@echo ""
	$(CC) $(CFLAGS) $< -o $@
	@$(SIZE) $@

%o : %cpp
	@echo ""
	@echo "CPP"
	@echo ""
	$(CC) $(CPFLAGS) $< -o $@
	@$(SIZE) $@

%o : %s
	@echo ""
	@echo "ASM"
	@echo ""
	$(AS) $(ASMFLAGS) $< -o $@
	@$(SIZE) $@

%elf: $(OBJS)
	@echo ""
	@echo "Linking"
	@echo ""
	$(LD) $(LDFLAGS) --output $@
	@$(SIZE) $(PROJECT).elf

%hex: %elf
	@echo ""
	@echo "Generating output HEX file"
	@$(OBJCOPY) $< --target=ihex $@

%bin: %elf
	@echo ""
	@echo "Generating output BIN file"
	@$(OBJCOPY) $< --output-target=binary $@

%dfu: %bin
	@echo ""
	@echo "Generating output DFU file"
	@DFU_Converter 8000000 $(PROJECT).bin

%dis: %elf
	@echo ""
	@echo "Generating DIS/BUG file"
	@$(OBJDUMP) -D -h -t $(PROJECT).elf > $(PROJECT).dis
	@$(OBJDUMP) -g $(PROJECT).elf > $(PROJECT).bug

%fdn: %bin
	@echo ""
	@echo "Flashing to target"
	@$(FLASHER) $(FLASHER_PARAM) $(PROJECT).bin $(FLASHER_ADDR) > $(PROJECT).fdn

#
# scripts
#

run_script:
	@$(PYTHON) create_handlers.py
	@$(PYTHON) create_vectors.py
	@$(PYTHON) create_boot.py

show_start:
	@echo "Starting ..."
	@$(CC) --version

show_finished:
	@echo ""
	@echo "Finished!"

exec_clean:
	@echo "Erasing all object files - $(ARCH)_$(MODEL)"
	@rm -f $(OBJS)
	@rm -f $(PROJECT).elf
	@rm -f $(PROJECT).map
	@rm -f $(PROJECT).hex
	@rm -f $(PROJECT).dis
	@rm -f $(PROJECT).bug
	@rm -f $(PROJECT).bin
	@rm -f $(PROJECT).fdn
	@rm -f ../boot/irq_handlers.c
	@rm -f ../boot/irq_vectors.s
	@rm -f ../boot/irq_boot.s

