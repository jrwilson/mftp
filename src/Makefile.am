AM_CXXFLAGS = -Wall -I$(top_srcdir)/include

LDADD = -lioa $(top_builddir)/lib/libmftp.la

bin_PROGRAMS = \
describe \
get

noinst_PROGRAMS = \
share \
specify \
udp_receiver \
udp_sender


describe_SOURCES = describe.cpp dd.hpp
get_SOURCES = get.cpp jam.hpp
share_SOURCES = share.cpp jam.hpp
specify_SOURCES = specify.cpp dd.hpp
udp_receiver_SOURCES = udp_receiver.cpp
udp_sender_SOURCES = udp_sender.cpp
