# BSD 3-Clause License
# 
# Copyright (c) 2022, Stable Cloud Computing, Inc.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# shared rules which must be included first in submakefiles

.PHONY: default
default: debug

.PHONY: cleanall
cleanall:
	rm -rf $(BASE)/sccbin/*
	rm -rf $(BASE)/sccobj/*

GNUMAKEFLAGS += --no-print-directory

# haswell arch is runs on all cloud provider VMs except Google N1.
# broadwell arch has the intel RDSEED instruction
# native is probably the default for GCC compiler
ifndef ARCH
ARCH := native
endif

# default is to use openssl library for cryptography
IPP := off

# choose minimal flags to get the job done

CXXFLAGS += -march=$(ARCH) -pipe -U_FORTIFY_SOURCE -fstack-protector -Wall -Wunused-but-set-parameter -Wno-free-nonheap-object -fno-omit-frame-pointer -std=c++20
CPPFLAGS += -fno-canonical-system-headers -Wno-builtin-macro-redefined '-D__DATE__="redacted"' '-D__TIMESTAMP__="redacted"' '-D__TIME__="redacted"'
LDFLAGS += -lstdc++ -pthread -lm

# all intermediate output goes to objdir
ifeq ($(BLDTYPE),debug)
CXXFLAGS += -O0 -g
OBJDIR = $(BASE)/sccobj/$(NAME)_d
else
# NDEBUG is defined to disable any assert() macros in production
CXXFLAGS += -g0 -O3 -DNDEBUG -ffunction-sections -fdata-sections
OBJDIR = $(BASE)/sccobj/$(NAME)
endif

ifeq ($(USE_GCOV),yes)
CPPFLAGS += --coverage -fprofile-arcs -ftest-coverage
GCOV_LD = -lgcov
endif

# all apps and shared libs go to bindir
BINDIR = $(BASE)/sccbin

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
