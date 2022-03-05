ifndef SCCLIB_UTIL
SCCLIB_UTIL := 1

BLDLIBS += $(BASE)/util

CPPFLAGS += -I $(BASE)/util/pub

ifeq ($(BLDTYPE),debug)
SLIBS := -lsccutild $(SLIBS)
else
SLIBS := -lsccutil $(SLIBS)
endif

endif
