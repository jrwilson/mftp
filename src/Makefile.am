AM_CXXFLAGS = -Wall -I$(top_srcdir)/include

#LDADD = $(top_builddir)/lib/libmftp.la
LDADD = -lioa

bin_PROGRAMS = \
get \
share

get_SOURCES = get.cpp mftp.hpp file.hpp sha2_256.hpp file.cpp mftp_sender_automaton.hpp mftp_receiver_automaton.hpp mftp_automaton.hpp
share_SOURCES = share.cpp mftp.hpp file.hpp sha2_256.hpp file.cpp mftp_sender_automaton.hpp mftp_receiver_automaton.hpp mftp_automaton.hpp
