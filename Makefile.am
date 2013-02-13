bin_PROGRAMS=
BUILT_SOURCES=
CLEANFILES=

AM_CPPFLAGS = $(MODULES_CFLAGS) -Irabbitmq-c-rabbitmq-c-v0.3.0/librabbitmq
#AM_LDFLAGS = -static
LDADD = $(MODULES_LIBS)

%.pb.h %.pb.cc: %.proto
	protoc -I. --cpp_out=. $<

# --------------------
# --- main ---
# --------------------

bin_PROGRAMS += main

main_SOURCES = main.cpp
main_SOURCES += rss.pb.cc

BUILT_SOURCES += rss.pb.h rss.pb.cc

main_LDADD = $(LDADD) -levent rabbitmq-c-rabbitmq-c-v0.3.0/librabbitmq/librabbitmq.a

# --------------------

CLEANFILES += $(BUILT_SOURCES)

rpm: all
	rpmbuild --buildroot `pwd`/buildroot --define="_topdir ." --define="_builddir ." -bb tryrabbitmq.spec
