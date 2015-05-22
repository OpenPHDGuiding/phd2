lib_LTLIBRARIES = libopenssag.la

libopenssag_la_CPPFLAGS = $(LIBUSB_CFLAGS)
if SHARED
libopenssag_la_LIBADD  = $(LIBUSB_LIBS)
endif
libopenssag_la_SOURCES = openssag.cpp loader.cpp
libopenssag_la_LDFLAGS = -no-undefined -version-info 0:0:0
noinst_HEADERS = firmware.h openssag_priv.h
include_HEADERS = openssag.h
