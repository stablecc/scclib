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
