AM_CPPFLAGS=
bin_PROGRAMS=
BUILT_SOURCES=
CLEANFILES=

AM_CPPFLAGS += $(MODULES_CFLAGS)

%.pb.h %.pb.cc: %.proto
	protoc -I. --cpp_out=. $<

BUILT_SOURCES += rss.pb.h rss.pb.cc

# --- main ---

bin_PROGRAMS += main

main_SOURCES = main.cpp
main_SOURCES += rss.pb.cc

main_LDADD =
main_LDADD += $(MODULES_LIBS)
main_LDADD += -lrabbitmq

CLEANFILES += $(BUILT_SOURCES)

rpm: all
	rpmbuild --buildroot `pwd`/buildroot --define="_topdir ." --define="_builddir ." -bb tryrabbitmq.spec
