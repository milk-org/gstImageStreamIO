# Note: plugindir is set in configure


if DEBUG
AM_CFLAGS = -g3 -O0
AM_CXXFLAGS = -g3 -O0
else
AM_CFLAGS = -O3
AM_CXXFLAGS = -O3
endif

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstgstImageStreamsrc.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstgstImageStreamsrc_la_SOURCES = gstImageStreamsrc.c gstImageStreamsrc.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstgstImageStreamsrc_la_CFLAGS = $(GST_CFLAGS) -O3 -fPIC -I/home/brule/cacao/ImageStreamIO/
libgstgstImageStreamsrc_la_LIBADD = $(GST_LIBS) 
libgstgstImageStreamsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -L/home/brule/cacao/ImageStreamIO/build -lImageStreamIO
libgstgstImageStreamsrc_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstImageStreamsrc.h
