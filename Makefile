BIN=curta.bin

GEN_ASM=a2e.stdjoy.s a2e.hi.s
SRCS=$(wildcard *.c)
ASM=$(filter-out $(GEN_ASM),$(wildcard *.s)) $(GEN_ASM)

C_OBJS=$(SRCS:.c=.o)
ASM_OBJS=$(ASM:.s=.o)
OBJS=$(C_OBJS) $(ASM_OBJS)

PLATFORM=apple2
#PLATFORM_CFG=-C $(PLATFORM)-loader.cfg
PLATFORM_CFG=--start-addr '$$4000'

MAPFILE=curta.map

all:    $(BIN)

# Big hack here.  The joystick library from cc65 only considers a
# jostick centered if it is within 20 of 127 which is very tight.
# Thus the tests for $6B and $93.  The good news is that the only
# occurrences of these bytes in the binary image are for these
# comparisons.  So, I just do a global search and replace.  The
# new threshold I am using is +/- 60 from 127.
a2e.stdjoy.s:
	co65 --code-label _a2e_stdjoy -o $@ $(CC65_HOME)/joy/a2e.stdjoy.joy
	sed -i .tmp 's/\$$6B/$$43/g' a2e.stdjoy.s
	sed -i .tmp 's/\$$93/$$BB/g' a2e.stdjoy.s

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
	cp $(BIN) ~/Documents/"Apple ]["/Transfer
