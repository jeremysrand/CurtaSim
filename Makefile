BIN=curta.bin
SYS=curta.system.sys

INSTALL_DIR=~/Documents/"Apple ]["/Transfer

GEN_ASM=a2e.hi.s
SRCS=$(wildcard *.c)
ASM=$(filter-out $(GEN_ASM),$(wildcard *.s)) $(GEN_ASM)

C_OBJS=$(SRCS:.c=.o)
ASM_OBJS=$(ASM:.s=.o)
OBJS=$(C_OBJS) $(ASM_OBJS)

PLATFORM=apple2
PLATFORM_CFG=--start-addr '$$4000'

MAPFILE=curta.map

all:    $(BIN)

a2e.hi.s:
	co65 --code-label _a2e_hi -o $@ $(CC65_HOME)/tgi/a2e.hi.tgi

%.o:	%.s
	ca65 -t $(PLATFORM) -o $@ $<

$(BIN): $(ASM_OBJS) $(SRCS)
	cl65 -t $(PLATFORM) $(PLATFORM_CFG) --mapfile $(MAPFILE) -o $(BIN).tmp $(SRCS) $(addprefix --obj ,$(ASM_OBJS)) 
	dd if=$(BIN).tmp of=$(BIN) bs=4 skip=1
	rm -f $(BIN).tmp

clean:
	rm -f $(BIN) $(OBJS) $(GEN_ASM) $(MAPFILE)

install: $(BIN)
	cp $(BIN) $(INSTALL_DIR)
	cp $(SYS) $(INSTALL_DIR)
