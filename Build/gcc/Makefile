# This Makefile builds the various components of the Orchestrator using gcc. It
# assumes you are running GNU/Linux, and have certain commonly-available GNU
# utilities installed (you'll know if you don't).

# Bash is used over sh for hashtables when building orchestrate (and probably
# other reasons).
SHELL = /bin/bash

# Smart directory creation.
MKDIR = mkdir --parents

# Software dependencies (MPI, Qt, etc.) are largely hardcoded, because I'm too
# short on time to write an autoconf wrapper or some other dependency
# management system.
#
# If you are a user, you should modify the Makefile.dependencies file in this
# directory to correctly path to the dependencies required to build the
# components of the Orchestrator. You should not need to modify this particular
# file.
#
# If you are a developer, you may also need to modify the following file to
# incorporate your additional sources:
include Makefile.dependencies

# Where sources live.
GENERICS_DIR := $(ROOT_DIR)/Generics
SOURCE_DIR := $(ROOT_DIR)/Source
TESTS_DIR := $(ROOT_DIR)/Tests

# Objects constructed from Orchestrator sources are placed in here. Objects are
# placed in a directory structure that mimics the source directory structure.
OBJECT_DIR := ./Objects

# Executable binaries are built to here (directory is created as executable
# binaries are built):
EXECUTABLE_DIR := $(ROOT_DIR)/bin

# Test binaries are built to here:
TEST_EXECUTABLE_DIR := $(ROOT_DIR)/test_bin

# The application staging environment is built here:
APPLICATION_STAGING_DIR := $(ROOT_DIR)/application_staging

# A note on dependency handling: we couple gcc's various -M flags with Make's
# ability to import dependency files to manage build dependencies. For more
# information,
# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#combine
#
# For context, we only want to rebuild objects if the source files that
# "affect" the object have changed. Maintaining a static list of dependencies
# (formally prerequisites), but this project is too big to make that a viable
# option, hence why I'm generating them at compilation time.
#
# In essence, when we build an object from a source file, we also create a
# Makefile dependency list for that source file if one already doesn't exist,
# at $(DEPENDENCY_DIR)/$*.temp.d. If the compilation is successful, we then
# move it to $(DEPENDENCY_DIR)/$*.d (see *Note 1*).
DEPENDENCY_DIR = ./Dependency_lists
DEPENDENCY_FLAGS = -MT "$@" -MMD -MP -MF $(DEPENDENCY_DIR)/$*.temp.d
$(shell $(MKDIR) $(DEPENDENCY_DIR))

# Include generated dependency files, because now is as good a time as any. We
# use find to do this, which is not the most portable way. If you can think of
# something better, let me know!
include $(shell find $(DEPENDENCY_DIR) -name "*.d" ! -name "*.temp.d")

# Where the compiler can look for header files when building Orchestrator
# objects:
INCLUDE_FLAGS = -I$(GENERICS_DIR) \
                -I$(SOURCE_DIR)/Common \
                -I$(SOURCE_DIR)/OrchBase \
                -I$(SOURCE_DIR)/OrchBase/HardwareFileReader \
                -I$(SOURCE_DIR)/OrchBase/HardwareConfigurationDeployment \
                -I$(SOURCE_DIR)/OrchBase/HardwareModel \
                -I$(SOURCE_DIR)/Launcher \
                -I$(SOURCE_DIR)/Injector \
                -I$(SOURCE_DIR)/Parser \
                -I$(SOURCE_DIR)/Root \
                -I$(SOURCE_DIR)/NameServer \
                -I$(SOURCE_DIR)/NameServer/AddressBook \
                -I$(SOURCE_DIR)/Softswitch/inc \
                -I$(SOURCE_DIR)/Supervisor \
                -I$(MPICH_INC_DIR) \
                -I$(QT_INC_DIR) \
                -I$(QT_INC_DIR)/QtCore \
                -I$(TINSEL_INC_DIR) \
                -I$(TINSEL_HOSTLINK_DIR)

# All flags to be used in compilation. We're not complying with C++98 yet due
# to the hostlink and some other minor changes, though nobody has studied this
# comprehensively.
#
# Trivial log handler is defined for now - we need to think about this more.
CXXFLAGS = $(DEPENDENCY_FLAGS) $(INCLUDE_FLAGS) \
           -std=c++11 -Wall -fPIC -pthread -pedantic -O3 -DTRIVIAL_LOG_HANDLER

# Debugging!
DEBUGFLAGS = -g3 -Og

# Debug target logic.
debug: CXXFLAGS += $(DEBUGFLAGS) -DORCHESTRATOR_DEBUG=1
debug: all

# Handy aliases for building executables, scripts, and libraries. Order doesn't
# matter.
ORCHESTRATE_BASENAME = orchestrate.sh
ORCHESTRATE_SCRIPT = $(ROOT_DIR)/$(ORCHESTRATE_BASENAME)

LAUNCHER_EXECUTABLE = $(EXECUTABLE_DIR)/orchestrate
ROOT_EXECUTABLE = $(EXECUTABLE_DIR)/root
DUMMY_EXECUTABLE = $(EXECUTABLE_DIR)/dummy
LOGSERVER_EXECUTABLE = $(EXECUTABLE_DIR)/logserver
LOGSERVER_MESSAGE_FILE = $(EXECUTABLE_DIR)/OrchestratorMessages.txt
RTCL_EXECUTABLE = $(EXECUTABLE_DIR)/rtcl
INJECTOR_EXECUTABLE = $(EXECUTABLE_DIR)/injector
MOTHERSHIP_EXECUTABLE = $(EXECUTABLE_DIR)/mothership

all: orchestrate launcher root dummy logserver rtcl injector mothership \
     application_staging_environment
launcher: $(LAUNCHER_EXECUTABLE)
orchestrate: $(ORCHESTRATE_SCRIPT)
root: $(ROOT_EXECUTABLE)
dummy: $(DUMMY_EXECUTABLE)
logserver: $(LOGSERVER_EXECUTABLE) $(LOGSERVER_MESSAGE_FILE)
rtcl: $(RTCL_EXECUTABLE)
injector: $(INJECTOR_EXECUTABLE)
mothership: $(MOTHERSHIP_EXECUTABLE)

# Prerequisites (objects) for building executables are defined here:
include Makefile.executable_prerequisites

# Prerequisites for building tests are defined here:
include Makefile.test_prerequisites
TEST_SCRIPT_BASENAME = run-tests.sh
TEST_SCRIPT = $(TEST_EXECUTABLE_DIR)/$(TEST_SCRIPT_BASENAME)
tests: CXXFLAGS += $(DEBUGFLAGS)
tests: $(ALL_TESTS) $(TEST_SCRIPT)

# Linking targets (to generate executables). See
# Makefile.executable_prerequisites for the prerequisites of these executables
# (obviously...)
$(LAUNCHER_EXECUTABLE) $(ROOT_EXECUTABLE) $(DUMMY_EXECUTABLE) \
$(LOGSERVER_EXECUTABLE) $(RTCL_EXECUTABLE) $(INJECTOR_EXECUTABLE):
	@$(shell $(MKDIR) $(EXECUTABLE_DIR))
	$(CXX) -pthread -Wl,-rpath-link=$(QT_LIB_DIR) \
        -L$(MPICH_LIB_DIR) -L$(QT_LIB_DIR) -L/usr/lib \
        -o $@ $^ \
        -lQt5Core -lmpi -lpthread

$(ALL_TESTS):
	@$(shell $(MKDIR) $(TEST_EXECUTABLE_DIR))
	$(CXX) -pthread -Wl,-rpath-link=$(QT_LIB_DIR) \
        -L$(MPICH_LIB_DIR) -L$(QT_LIB_DIR) -L/usr/lib \
        -o $@ $^ \
        -lQt5Core -lmpi -lpthread

$(TEST_SCRIPT): $(TEST_SCRIPT_RESOURCE)
	@$(shell $(MKDIR) $(TEST_EXECUTABLE_DIR))
	cp "$(TEST_SCRIPT_RESOURCE)" "$(TEST_SCRIPT)"

$(MOTHERSHIP_EXECUTABLE):
	@$(shell $(MKDIR) $(EXECUTABLE_DIR))
	$(CXX) -pthread -Wl,-rpath-link=$(QT_LIB_DIR) \
        -Wl,-export-dynamic \
        -L$(MPICH_LIB_DIR) -L$(QT_LIB_DIR) -L/usr/lib \
        -o $@ $^ $(TINSEL_HOSTLINK_DIR)/*.o \
        -lQt5Core -lmpi -lpthread -ldl

# Object generation. Note: The move command installs the assembled dependency
# file (see *Note 1*). The touch updates the timestamp on the target (to avoid
# pointless rebuilds), and is predicated on the move command succeeding.
define build-object
    @$(MKDIR) $$(dirname $@)
    $(CXX) $(CXXFLAGS) -c -o $@ $<
    @mv --force $(DEPENDENCY_DIR)/$*.temp.d $(DEPENDENCY_DIR)/$*.d
    @touch $@
endef

$(OBJECT_DIR)/%.o: $(ROOT_DIR)/%.cpp $(DEPENDENCY_DIR)/%.d
	$(build-object)

# Files to be copied to the execution directory in order for various components
# of the Orchestrator to function.
$(LOGSERVER_MESSAGE_FILE): $(LOGSERVER_MESSAGE_FILE_ORIGIN)
	@$(MKDIR) $$(dirname $@)
	cp "$<" "$@"

# Building the orchestrate script, used to setup the runtime environment for
# the Orchestrator, and to start it. The build process is largely
# templating. This build target uses a target-specific variable to determine
# the temporary path for building the orchestrate script.
#
# This logic assumes Bash 4 for the SUBSTITUTIONS hashtable, but that's not too
# unrealistic in this day and age.
$(ORCHESTRATE_SCRIPT): ORCHESTRATE_TMP:=$(shell mktemp --directory)/$(ORCHESTRATE_BASENAME)
$(ORCHESTRATE_SCRIPT): $(ORCHESTRATE_TEMPLATE)
	@# Create staging file space for subtitutions.
	cp "$<" "$(ORCHESTRATE_TMP)"

	@# Create hashtable and perform substitutions, all in the same shell.
	declare -A SUBSTITUTIONS=(\
        ["MPICH_DIR"]="$(realpath $(MPICH_DIR))" \
		["MPICH_LIB_DIR"]="$(realpath $(MPICH_LIB_DIR))" \
		["QT_LIB_DIR"]="$(realpath $(QT_LIB_DIR))" \
		["GCC_LIB_DIR"]="$(realpath $(GCC_LIB_DIR))" \
		["CR_LIB_DIR"]="$(realpath $(CR_LIB_DIR))" \
		["RISCV_DIR"]="$(realpath $(RISCV_DIR))" \
		["RISCV_BIN_DIR"]="$(realpath $(RISCV_BIN_DIR))" \
		["EXECUTABLE_DIR"]="$$(realpath -m $(EXECUTABLE_DIR))");\
	for KEY in "$${!SUBSTITUTIONS[@]}"; do \
		sed -i "s|{{ $$KEY }}|$${SUBSTITUTIONS[$$KEY]}|" "$(ORCHESTRATE_TMP)";\
	done

	@# Deploy
	cp "$(ORCHESTRATE_TMP)" "$@"

	@# Remove staging file and directory, carefully.
	rm "$(ORCHESTRATE_TMP)"
	rmdir "$(dir $(ORCHESTRATE_TMP))"

# Building the staging environment for XML applications to be used in the
# Orchestrator. Users will place their XML applications in the xml directory
# within the structure created by this process.
application_staging_environment: $(APPLICATION_STAGING_DIR)/xml \
$(APPLICATION_STAGING_DIR)/Orchestrator/Source \
$(APPLICATION_STAGING_DIR)/Orchestrator/Generics \
$(APPLICATION_STAGING_DIR)/Orchestrator/Source/Common \
$(APPLICATION_STAGING_DIR)/Softswitch \
$(APPLICATION_STAGING_DIR)/Tinsel \
$(APPLICATION_STAGING_DIR)/Makefile \
$(APPLICATION_STAGING_DIR)/Supervisor.cpp \
$(APPLICATION_STAGING_DIR)/Supervisor.h

$(APPLICATION_STAGING_DIR)/Orchestrator/Source \
$(APPLICATION_STAGING_DIR)/xml:
	$(MKDIR) "$@"

$(APPLICATION_STAGING_DIR)/Orchestrator/Generics:
	$(MKDIR) "$(dir $@)"
	ln --force --symbolic "$(abspath $(GENERICS_DIR))" "$@"

$(APPLICATION_STAGING_DIR)/Orchestrator/Source/Common:
	$(MKDIR) "$(dir $@)"
	ln --force --symbolic "$(abspath $(SOURCE_DIR)/Common)" "$@"

$(APPLICATION_STAGING_DIR)/Softswitch:
	$(MKDIR) "$(dir $@)"
	ln --force --symbolic "$(abspath $(SOURCE_DIR)/Softswitch)" "$@"

$(APPLICATION_STAGING_DIR)/Tinsel:
	$(MKDIR) "$(dir $@)"
	ln --force --symbolic "$(abspath $(TINSEL_DIR))" "$@"

$(APPLICATION_STAGING_DIR)/Makefile:
	$(MKDIR) "$(dir $@)"
	ln --force --symbolic "$(abspath $(SOURCE_DIR)/Softswitch/Makefile)" "$@"

$(APPLICATION_STAGING_DIR)/Supervisor.cpp $(APPLICATION_STAGING_DIR)/Supervisor.h:
	$(MKDIR) "$(dir $@)"
	ln --force --symbolic \
		"$(abspath $(SOURCE_DIR)/Supervisor/Supervisor$(suffix $@))" "$@"

# Tearing down the environment (for 'clean'), leaving the xml directory for the
# user. Only tear it down if $(APPLICATION_STAGING_DIR) exists. NB:
#
#  - Single quotes needed for the regex predicate, because the path is
#    relative. Consequently, the "." from "../../" should not be interpreted as
#    a character.
application_staging_environment_teardown:
	if [ -d "$(APPLICATION_STAGING_DIR)" ]; then \
		find "$(APPLICATION_STAGING_DIR)" \
			! -regex '$(APPLICATION_STAGING_DIR)/xml.*' \
			! -wholename "$(APPLICATION_STAGING_DIR)" \
			-exec rm --force --recursive {} +; \
	fi

# Dependency files have no build instructions (Make must not fall over if the
# dependency file is not there), because they are created from the compilation
# process. However, the directory structure for dependency files must be
# created.
$(DEPENDENCY_DIR)/%.d:
	@$(MKDIR) $$(dirname $@)

clean: application_staging_environment_teardown
	rm --force --recursive $(OBJECT_DIR) $(DEPENDENCY_DIR) $(EXECUTABLE_DIR) \
	$(ORCHESTRATE_SCRIPT) $(TEST_EXECUTABLE_DIR)

# Dependency files must not be removed by Make (unless explicitly cleaned).
.PRECIOUS: $(DEPENDENCY_DIR)/%.d

# Non-builtin targets that do not explicitly represent files that are created.
.PHONY: all clean debug tests orchestrate root dummy logserver rtcl injector \
    mothership application_staging_environment \
    application_staging_environment_teardown

# The "all" target should be the default.
.DEFAULT_GOAL := all
