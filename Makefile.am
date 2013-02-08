LDADD=
AM_CPPFLAGS=
bin_PROGRAMS=
BUILT_SOURCES=
CLEANFILES=

LDADD += $(MODULES_LIBS)
AM_CPPFLAGS += $(MODULES_CFLAGS) -Irabbitmq-c-rabbitmq-c-v0.3.0/librabbitmq

%.pb.h %.pb.cc: %.proto
	protoc -I. --cpp_out=. $<

BUILT_SOURCES += rss.pb.h rss.pb.cc

# --- main ---

bin_PROGRAMS += main

main_SOURCES = main.cpp
main_SOURCES += rss.pb.cc

main_LDADD = -levent rabbitmq-c-rabbitmq-c-v0.3.0/librabbitmq/librabbitmq.a $(LDADD)

CLEANFILES += $(BUILT_SOURCES)

rpm: all
	rpmbuild --buildroot `pwd`/buildroot --define="_topdir ." --define="_builddir ." -bb tryrabbitmq.spec
