AM_CXXFLAGS = -Wall -I$(top_srcdir)/include

#LDADD = $(top_builddir)/lib/libmftp.la
LDADD = -lioa

bin_PROGRAMS = \
get \
share

get_SOURCES = mftp.hpp file.hpp sha2_256.hpp file.cpp mftp_automaton.hpp conversion_channel_automaton.hpp get.cpp
share_SOURCES = share.cpp file.cpp mftp_automaton.hpp file.hpp mftp.hpp sha2_256.hpp conversion_channel_automaton.hpp
