
if GST_VERSION_1_0
  libgzdec_la_SOURCES = gstgzdec.c 
else
  libgzdec_la_SOURCES = gstgzdec0.1.c
endif

lib_LTLIBRARIES = libgzdec.la

libgzdec_la_CFLAGS  =  $(GST_CFLAGS)
libgzdec_la_LDFLAGS = $(ZLIB_LIBS) $(GST_LIBS)

noinst_HEADERS = gstgzdec.h