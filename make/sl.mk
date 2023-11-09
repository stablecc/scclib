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

# submakefile for a shared library to be included last in Makefile

CPPFLAGS += -fpic

include $(BASE)/scclib/make/shared.mk

.PHONY: debug
debug:
	@for lib in $(SLBUILDS); do \
	$(MAKE) -C $$lib debug; \
	done
	$(MAKE) BLDTYPE=debug $(BINDIR)/lib$(NAME)d.so

.PHONY: gcov
gcov:
	@for lib in $(SLBUILDS); do \
	$(MAKE) -C $$lib debug; \
	done
	$(MAKE) BLDTYPE=debug USE_GCOV=yes $(BINDIR)/lib$(NAME)d.so

.PHONY: release
release:
	@for lib in $(SLBUILDS); do \
	$(MAKE) -C $$lib release; \
	done
	$(MAKE) BLDTYPE=release $(BINDIR)/lib$(NAME).so

.PHONY: clean
clean:
	rm -f $(BINDIR)/lib$(NAME).so
	rm -f $(BINDIR)/lib$(NAME)d.so
	rm -rf $(OBJDIR)
	rm -rf $(OBJDIR)_d

.PHONY: cleanlibs
cleanlibs: clean
	@for lib in $(SLBUILDS); do \
	$(MAKE) -C $$lib cleanlibs; \
	done

.PHONY: help
help:
	@echo
	@echo 'build targets'
	@echo '-------------'
	@echo
	@echo 'debug    - builds with debugging information and no optimization'
	@echo 'release  - builds release version with full optimization'
	@echo
	@echo 'code coverage build creates a debug target with --coverage and -profile-arcs'
	@echo 'to cleanly use coverage, it is best to cleanall before and after'
	@echo
	@echo 'gcov     - builds the library with debug and --coverage and -fprofile-arcs'
	@echo
	@echo 'make ARCH=haswell <target>  - builds for Amazon T2 instances or better'
	@echo 'make ARCH=broadwell <target>  - builds for with the RDSEED instruction enabled'
	@echo
	@echo 'clean      - clean this module only'
	@echo 'cleanlibs  - clean all shared libs for this module'
	@echo "cleanall   - clean everything by removing ${BASE}/sccbin and ${BASE}/sccobj directory contents"
	@echo 

include $(BASE)/scclib/make/rules.mk
