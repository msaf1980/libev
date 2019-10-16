AUTOMAKE_OPTIONS = foreign

proj_dir = ${top_srcdir}/..

AM_CPPFLAGS = -I${proj_dir}/
AM_LDFLAGS = -lz -lpthread

noinst_PROGRAMS = \
test_connect 
    
TESTS = $(bin_PROGRAMS)

test_connect_SOURCES = test_connect.c
#test_connect_LIDADD = libev.la
