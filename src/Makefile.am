AM_CXXFLAGS = -Wall -I$(top_srcdir)/include

LDADD = -lioa $(top_builddir)/lib/libmftp.la

bin_PROGRAMS = \
describe \
get \
share \
specify

describe_SOURCES = describe.cpp dd.hpp
get_SOURCES = get.cpp jam.hpp
share_SOURCES = share.cpp jam.hpp
specify_SOURCES = specify.cpp dd.hpp
