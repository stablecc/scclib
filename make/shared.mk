# shared rules which must be included first in submakefiles

.PHONY: default
default: debug

.PHONY: cleanall
cleanall:
	rm -rf $(BASE)/bin/*
	rm -rf $(BASE)/obj/*

GNUMAKEFLAGS += --no-print-directory

# haswell arch is runs on all cloud provider VMs except Google N1.
# broadwell arch has the intel RDSEED instruction
# native is probably the default for GCC compiler
ifndef ARCH
ARCH := native
endif

# choose minimal flags to get the job done

CPPFLAGS += -march=$(ARCH) -pipe
CXXFLAGS += -std=c++17
LDFLAGS += -lstdc++ -pthread -lm

# all intermediate output goes to objdir
ifeq ($(BLDTYPE),debug)
CPPFLAGS += -Wall -O0 -g
OBJDIR = $(BASE)/obj/$(NAME)_d
else
# NDEBUG is defined to disable any assert() macros in production
# production uses maximum optimization, probably not recommended
CPPFLAGS += -O3 -DNDEBUG
OBJDIR = $(BASE)/obj/$(NAME)
endif

ifeq ($(USE_GCOV),yes)
CPPFLAGS += --coverage -fprofile-arcs -ftest-coverage
GCOV_LD = -lgcov
endif

# all apps and shared libs go to bindir
BINDIR = $(BASE)/bin

# if needed, create subdirs under the objdir directory for all source files
OBJS = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(SRCS))))

# only build directories on a real build
ifdef BLDTYPE

# create a list of directories to build
# remove duplicates and put in order
DIRMK = $(sort $(dir $(abspath $(OBJS))))
XOBJDIR = $(abspath $(OBJDIR))/

$(shell mkdir -p $(OBJDIR) &> /dev/null)
$(shell mkdir -p $(BINDIR) &> /dev/null)

# if the first (shortest) dir is below objdir, this is an error
ifeq ($(findstring $(XOBJDIR), $(word 1, $(DIRMK))),$(XOBJDIR))
$(shell mkdir -p $(DIRMK) &> /dev/null)
else
$(error Sources contain a directory outside of build tree)
endif

endif
