ifndef SCCLIB_NET
SCCLIB_NET := 1

BLDLIBS += $(BASE)/net

CPPFLAGS += -I $(BASE)/net/pub

include $(BASE)/util/make.mk

ifeq ($(BLDTYPE),debug)
SLIBS := -lsccnetd $(SLIBS)
else
SLIBS := -lsccnet $(SLIBS)
endif

endif
