#
#  tail.mk
#  Apple2BuildPipelineSample
#
#  Part of a sample build pipeline for Apple ][ software development
#
#  Created by Quinn Dunki on 8/15/14.
#  One Girl, One Laptop Productions
#  http://www.quinndunki.com
#  http://www.quinndunki.com/blondihacks
#

export PATH := $(PATH):$(CC65_BIN)

C_OBJS=$(C_SRCS:.c=.o)
C_DEPS=$(C_SRCS:.c=.u)
ASM_OBJS=$(ASM_SRCS:.s=.o)
ASM_LSTS=$(ASM_SRCS:.s=.lst)
OBJS=$(C_OBJS) $(ASM_OBJS)

MAPFILE=$(PGM).map
DISKIMAGE=$(PGM).dsk

LINK_ARGS=

EXECCMD=

ifneq ($(START_ADDR),)
# If the MACHINE is set to an option which does not support a variable start
# address, then error.
    ifneq ($(filter $(MACHINE), apple2-system apple2enh-system),)
        $(error You cannot change start address with this machine type)
    endif
    LDFLAGS += --start-addr 0x$(START_ADDR)
endif

ifneq ($(filter $(MACHINE), apple2 apple2enh apple2-dos33 apple2enh-dos33),)
    EXECCMD=$(shell echo brun $(PGM) | tr '[a-z]' '[A-Z]')
endif

MACHCONFIG= -t apple2

ifneq ($(filter $(MACHINE), apple2enh apple2apple2enh-dos33 apple2enh-system apple2enh-loader apple2enh-reboot),)
    MACHCONFIG= -t apple2enh
endif

ifeq ($(filter $(MACHINE), apple2 apple2enh),)
    MACHCONFIG += -C $(MACHINE).cfg
endif

.PHONY: all execute clean
	
all: execute

clean:
	rm -f $(PGM)
	rm -f $(OBJS)
	rm -f $(C_DEPS)
	rm -f $(MAPFILE)
	rm -f $(ASM_LSTS)
	rm -f $(DISKIMAGE)

$(PGM): $(OBJS)
	$(CL65) $(MACHCONFIG) --mapfile $(MAPFILE) $(LDFLAGS) -o $(PGM) $(OBJS)

$(DISKIMAGE): $(PGM)
	make/createDiskImage $(AC) $(MACHINE) $(DISKIMAGE) $(PGM)

execute: $(DISKIMAGE)
	osascript make/V2Make.scpt $(PROJECT_DIR) $(PGM) $(PROJECT_DIR)/DevApple.vii "$(EXECCMD)"

%.o:	%.c
	$(CL65) $(MACHCONFIG) $(CFLAGS) --create-dep -c -o $@ $<
	sed -i .bak 's/\.s:/.o:/' $(@:.o=.u)
	rm -f $(@:.o=.u).bak

%.o:	%.s
	$(CL65) $(MACHCONFIG) --cpu $(CPU) $(ASMFLAGS) -l -c -o $@ $<

-include $(C_DEPS)