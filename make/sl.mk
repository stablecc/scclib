# submakefile for a shared library to be included last in Makefile

CPPFLAGS += -fpic

include $(BASE)/make/shared.mk

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
	@echo "cleanall   - clean everything by removing ${BASE}/bin and ${BASE}/obj directory contents"
	@echo 

include $(BASE)/make/rules.mk
