# Makefile for notify/none plugin

AUTOMAKE_OPTIONS = foreign

PLUGIN_FLAGS = -module -avoid-version --export-dynamic

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src/common

pkglib_LTLIBRARIES = notify_none.la

# none notify plugin.
notify_none_la_SOURCES = notify_none.c
notify_none_la_CPPFLAGS = $(none_CPPFLAGS)
notify_none_la_LDFLAGS = $(SO_LDFLAGS) $(PLUGIN_FLAGS)
