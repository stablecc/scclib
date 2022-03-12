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

# rules to build object files which must be included last in submakefiles

DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = @mv -f $(OBJDIR)/$*.Td $(OBJDIR)/$*.d

# if we don't mark these objects as secondary, they get removed as intermediate by the make system
# and cause dependancy syncronization problems

.SECONDARY: $(OBJS)

# try to support most ways of naming c/c++ files

$(OBJDIR)/%.o : %.c
$(OBJDIR)/%.o : %.c $(OBJDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o : %.cc
$(OBJDIR)/%.o : %.cc $(OBJDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o : %.cxx
$(OBJDIR)/%.o : %.cxx $(OBJDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o : %.cpp
$(OBJDIR)/%.o : %.cpp $(OBJDIR)/%.d
	echo $(OUTPUT_OPTION)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

# shared libs do not need library includes

$(BINDIR)/%.so: $(OBJS)
	$(CC) -shared -Wl,-soname,$(notdir $@) -o $@ $(OBJS) $(GCOV_LD)
	@echo Built shared library $@

$(BINDIR)/%: $(OBJS)
	$(CC) -o $@ $(OBJS) $(GCOV_LD) $(LDFLAGS) -L $(BINDIR) $(SLIBS) 
	@echo Built application $@

# precious means that if the make is interrupted, the targets are not deleted
# this removes a condition where dependancy files get out of sync with object files

$(OBJDIR)/%.d: ;
.PRECIOUS: $(OBJDIR)/%.d

include $(wildcard $(patsubst %,$(OBJDIR)/%.d,$(basename $(SRCS))))
