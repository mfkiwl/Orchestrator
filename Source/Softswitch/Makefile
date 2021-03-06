# Main components roots
export TINSEL_ROOT=../Tinsel
export SOFTSW_ROOT=../Softswitch
# this needs to be modified to reflect actual codebase location
export ORCH_ROOT=../Orchestrator

# Common directories
TINSELINC = $(realpath $(TINSEL_ROOT)/include)
TINSELLIB = $(realpath $(TINSEL_ROOT)/lib)
TINSELBIN = $(realpath $(TINSEL_ROOT)/bin)
TINSELHL  = $(realpath $(TINSEL_ROOT)/hostlink)

SOFTSWINC = $(realpath $(SOFTSW_ROOT)/inc)
SOFTSWSRC = $(realpath $(SOFTSW_ROOT)/src)

PCOMMONINC = $(realpath $(ORCH_ROOT)/Source/Common)
GENERICSINC = $(realpath $(ORCH_ROOT)/Generics)

ifndef RISCV_PATH
export RISCV_PATH := /usr/local/riscv
endif

ifndef MPICH_PATH
export MPICH_PATH := /usr/local/mpich
endif

# Application directories
DEFAULT_GEN_DIR = Generated
ifndef GENINC
GENINC := $(realpath ../$(DEFAULT_GEN_DIR)/inc)
endif
ifndef GENSRC
GENSRC := $(realpath ../$(DEFAULT_GEN_DIR)/src)
endif
ifndef BIN_DIR
BIN_DIR := ../bin
endif

ifdef TRIVIAL_LOG_HANDLER
LH_CFLAGS := $(LH_CFLAGS) -DTRIVIAL_LOG_HANDLER
endif

ifdef DISABLE_SOFTSWITCH_INSTRUMENTATION
LH_CFLAGS := $(LH_CFLAGS) -DDISABLE_SOFTSWITCH_INSTRUMENTATION
endif

ifdef SOFTSWITCH_PRIORITISE_INSTRUMENTATION
LH_CFLAGS := $(LH_CFLAGS) -DSOFTSWITCH_PRIORITISE_INSTRUMENTATION
endif

# MPI tools
MPICHINC = $(MPICH_PATH)/include
MPICHBIN = $(MPICH_PATH)/bin
MPICHLIB = $(MPICH_PATH)/lib
MPICC    = $(MPICHBIN)/mpicc
MPICXX   = $(MPICHBIN)/mpicxx

# RISC-V tools
RV_ARCH     = rv32imf
RV_CC       = riscv32-unknown-elf-gcc
RV_CPPC     = riscv32-unknown-elf-g++
RV_LD       = riscv32-unknown-elf-ld
RV_OBJCOPY  = riscv32-unknown-elf-objcopy
RV_LIBS     = $(RISCV_PATH)/riscv32-unknown-elf/lib/rv32im/ilp32
# the gcc version number (path component after riscv32-unknown elf needs to
# be set according to the target system's build environment
RV_GCC_LIBS = $(RISCV_PATH)/lib/gcc/riscv32-unknown-elf/8.2.0/rv32im/ilp32
# this had -nostdlib; we need it for memcpy.
RV_CFLAGS   = -mabi=ilp32 -march=$(RV_ARCH) -static -mcmodel=medany \
              -fvisibility=hidden -nostartfiles -pipe\
              -fsingle-precision-constant -fno-builtin-printf

# Extend PATH
export PATH := $(PATH):$(TINSEL_ROOT):$(realpath $(TINSELBIN)):$(SOFTSWINC):$(RISCV_PATH)/bin

# Set path to config script
export CONFIG := $(realpath $(TINSEL_ROOT)/config.py)

# Set make path for prerequisites
# generated files look (first) in the generation directory
vpath vars_%.cpp $(GENSRC)
vpath handlers_%.cpp $(GENSRC)
vpath vars_%.h $(GENINC)
vpath handlers_%.h $(GENINC)
vpath libc.a $(RV_LIBS)
vpath libgcc.a $(RV_GCC_LIBS)
vpath mpi%.h $(MPICHINC)
vpath opa_%.h $(MPICHINC)
vpath PMsg_p.hpp $(PCOMMONINC)
vpath poets_pkt.h $(PCOMMONINC)
vpath OSFixes.hpp $(PCOMMONINC)
vpath Supervisor.% $(realpath ../$(DEFAULT_GEN_DIR))
VPATH := $(SOFTSWSRC):$(SOFTSWINC):$(TINSELLIB):$(TINSELINC):$(GENERICSINC)

# Local compiler flags
CFLAGS = $(RV_CFLAGS) $(LH_CFLAGS) -O2 -I $(TINSELINC) -I $(SOFTSWINC) -I $(GENINC) -I $(PCOMMONINC) -D__INSTRUMENTATION
LDFLAGS = -melf32lriscv -G 0 -L$(RV_LIBS) -L$(RV_GCC_LIBS) -lgcc -lc

MPICFLAGS = -I$(SOFTSWINC) -I$(TINSELINC) -I$(MPICHINC) -I$(PCOMMONINC) -I$(GENERICSINC) -D__INSTRUMENTATION -std=c++11 -fPIC -pipe -Wall
MPILDFLAGS = -shared -L$(MPICHLIB)

SECEXPCPPS := $(GENSRC)/%.cpp
SECEXPOS := %.o

# binary targets
TARGETS := $(patsubst $(GENINC)/vars_%.h,$(BIN_DIR)/softswitch_%.elf,$(wildcard $(GENINC)/vars_*.h))
VCTARGETS := $(patsubst $(GENINC)/vars_%.h,$(BIN_DIR)/softswitch_code_%.v,$(wildcard $(GENINC)/vars_*.h))
VDTARGETS := $(patsubst $(GENINC)/vars_%.h,$(BIN_DIR)/softswitch_data_%.v,$(wildcard $(GENINC)/vars_*.h))
# targets for thread-local variables
THREADVARS := $(patsubst $(GENSRC)/%.cpp,%.o,$(wildcard $(GENSRC)/vars_*_*.cpp))

.PHONY: all clean printvars supervisor
all : $(TARGETS) $(VCTARGETS) $(VDTARGETS) supervisor

# This line can be given a target/prerequisite (tgt : $(TARGETS)) if needed
#	$(foreach TARGET,$<,cp $(TARGET) $(BIN_DIR)/$(TARGET))

supervisor: $(BIN_DIR)/libSupervisor.so

$(BIN_DIR)/libSupervisor.so: Supervisor.cpp Supervisor.h
	$(MPICXX) $(MPICFLAGS) $(MPILDFLAGS) -Wl,-soname,libSupervisor.so -o $@ $<

$(BIN_DIR)/softswitch_code_%.v: $(BIN_DIR)/softswitch_%.elf
	$(TINSELBIN)/checkelf.sh $<
	$(RV_OBJCOPY) -O verilog --only-section=.text $< $@

$(BIN_DIR)/softswitch_data_%.v: $(BIN_DIR)/softswitch_%.elf
	$(RV_OBJCOPY) -O verilog --remove-section=.text --set-section-flags .bss=alloc,load,contents $< $@

softswitch_%.o : softswitch_%.cpp softswitch_common.h softswitch.h poets_pkt.h poets_hardware.h tinsel.h $(TINSELINC)/config.h $(SOFTSWITCH_DEPS)
	$(RV_CPPC) $(CFLAGS) -Wall -c -o $@ $<

softswitch.o : softswitch.cpp softswitch.h poets_pkt.h poets_hardware.h tinsel.h $(TINSELINC)/config.h $(SOFTSWITCH_DEPS)
	$(RV_CPPC) $(CFLAGS) -Wall -c -o $@ $<

io.o : io.c io.h tinsel.h $(TINSELINC)/config.h
	$(RV_CPPC) $(CFLAGS) -Wall -c -o $@ $<

## the pattern substitution should find all vars_x_y.o created from vars_x_y.cpp
.SECONDEXPANSION:
$(BIN_DIR)/softswitch_%.elf : softswitch_main.o softswitch_common.o softswitch.o vars_%.o $$(patsubst $$(SECEXPCPPS),$$(SECEXPOS),$$(wildcard $(GENSRC)/vars_$$*_*.cpp)) handlers_%.o io.o link_%.ld entry.o libc.a libgcc.a
	$(RV_LD) $(LDFLAGS) -T link_$(notdir $*).ld -o $@ $(filter-out link_$(notdir $*).ld, $^)

# this convoluted rule (we hope) will establish as a prerequisite for any file
# vars_x_y.cpp to be compiled to vars_x_y.o a header file vars_x.h. We do this
# by a trick: get the c++ file in question via the automatic-variable-substituted
# filename for the subst expression, then use an inner substitution to replace
# the stem (the part x_y) with x.y and run basename to strip off the part after
# the dot. (If basename tries to get clever about recognising 'valid' filenames
# we can append the vars_ prefix after the inner substitution and/or replace the
# _ with .a instead of .)
$(THREADVARS) : vars_%.o: vars_%.cpp $$(subst $$*.cpp,$$(basename $$(subst _,.,$$*)).h,vars_$$*.cpp)
	$(RV_CPPC) $(CFLAGS) -Wall -c -o $@ $<

vars_%.o : vars_%.cpp vars_%.h
	$(RV_CPPC) $(CFLAGS) -Wall -c -o $@ $<

handlers_%.o : handlers_%.cpp handlers_%.h vars_%.h
	$(RV_CPPC) $(CFLAGS) -Wall -c -o $@ $<

entry.o : entry.S
	$(RV_CC) $(CFLAGS) -Wall -c -o $@ $<

link_%.ld : genld.sh
	$< $(notdir $*) > link_$(notdir $*).ld

$(HL)/%.o :
	make -C $(TINSELHL)

clean :
	rm -f *.o *.elf link.ld $(BIN_DIR)/*.elf $(BIN_DIR)/libSupervisor.so
	make clean -C $(TINSELINC)
