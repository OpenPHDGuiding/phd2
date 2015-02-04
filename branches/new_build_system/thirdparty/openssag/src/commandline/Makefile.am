if ENABLE_TOOL
noinst_PROGRAMS = openssag
openssag_CPPFLAGS = $(LIBUSB_CFLAGS) $(MAGICKCORE_CFLAGS)
openssag_LDADD = ../libopenssag.la $(LIBUSB_LIBS) $(MAGICKCORE_LIBS)
openssag_SOURCES = ssag.cpp
endif
