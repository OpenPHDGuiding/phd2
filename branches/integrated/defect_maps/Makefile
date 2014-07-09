# makefile for Linux . Can be used as alternative to CMake

all: phd2

WX_CPPFLAGS ?= $(shell wx-config --cxxflags)
WX_LDLIBS   ?= $(shell wx-config --libs core,base,net,html,aui,adv)
FITS_LDLIBS ?= $(shell pkg-config cfitsio --libs)
FITS_CFLAGS ?= $(shell pkg-config cfitsio --cflags)
CXXFLAGS ?= -O2
CPPFLAGS ?= -Wall -Werror
CPPFLAGS += $(WX_CPPFLAGS) $(FITS_CFLAGS)
# FIXME: fix camera negotiation. Hardcoded lists don't make much sense
CPPFLAGS += -DSIMULATOR
LDLIBS  ?= $(WX_LDLIBS) $(FITS_LDLIBS)
LDLIBS += -lusb-1.0

CXX_SRC = $(wildcard *.cpp)

## Omit some stuff for a start:
## cam_firewire_OSX needs fixing to use libdc1394-22-dev proper
#CXX_SRC := $(filter-out cam_firewire_OSX.cpp,$(CXX_SRC))
#CXX_SRC := $(filter-out cam_QHY5IIbase.cpp,$(CXX_SRC))

CXX_OBJ = $(CXX_SRC:.cpp=.o)

phd2: $(CXX_OBJ)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -f $(CXX_OBJ) phd2
