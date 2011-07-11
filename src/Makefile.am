AM_CXXFLAGS = -Wall -I$(top_srcdir)/include

LDADD = -lioa $(top_builddir)/lib/libmftp.la

bin_PROGRAMS = \
get \
share

get_SOURCES = get.cpp jam.hpp
share_SOURCES = share.cpp jam.hpp
