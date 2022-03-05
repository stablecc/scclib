# submakefile for an application to be included last in Makefile

include $(BASE)/make/shared.mk

.PHONY: debug
debug:
	@for lib in $(BLDLIBS); do \
	$(MAKE) -C $$lib debug; \
	done
	$(MAKE) BLDTYPE=debug $(BINDIR)/$(NAME)_d
	@echo Run with cmd: LD_LIBRARY_PATH=$(BINDIR) $(BINDIR)/$(NAME)_d

.PHONY: gcov
gcov:
	@for lib in $(BLDLIBS); do \
	$(MAKE) USE_GCOV=yes -C $$lib debug; \
	done
	$(MAKE) BLDTYPE=debug USE_GCOV=yes $(BINDIR)/$(NAME)_d
	@echo Run with cmd: LD_LIBRARY_PATH=$(BINDIR) $(BINDIR)/$(NAME)_d

.PHONY: release
release:
	@for lib in $(BLDLIBS); do \
	$(MAKE) -C $$lib release; \
	done
	$(MAKE) BLDTYPE=release $(BINDIR)/$(NAME)
	@echo Run with cmd: LD_LIBRARY_PATH=$(BINDIR) $(BINDIR)/$(NAME)

.PHONY: clean
clean:
	rm -f $(BINDIR)/$(NAME)
	rm -f $(BINDIR)/$(NAME)_d
	rm -rf $(OBJDIR)
	rm -rf $(OBJDIR)_d

.PHONY: cleanlibs
cleanlibs: clean
	@for lib in $(BLDLIBS); do \
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
	@echo 'code coverage build creates a debug target with --coverage -fprofile-arcs -ftest-coverage'
	@echo 'to cleanly use coverage, it is best to cleanall before and after'
	@echo
	@echo 'gcov     - builds the debug executable with coverage enabled'
	@echo
	@echo 'make ARCH=haswell <target>  - builds for Amazon T2 instances or better'
	@echo 'make ARCH=broadwell <target>  - builds for with the RDSEED instruction enabled'
	@echo
	@echo 'clean      - clean this module only'
	@echo 'cleanlibs  - clean all shared libs for this module'
	@echo "cleanall   - clean everything by removing ${BASE}/bin and ${BASE}/obj directory contents"
	@echo 

include $(BASE)/make/rules.mk
