# Makefile for notify/ftb plugin

AUTOMAKE_OPTIONS = foreign

PLUGIN_FLAGS = -module -avoid-version --export-dynamic

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src/common

pkglib_LTLIBRARIES = notify_ftb.la

# FTB notify plugin.
notify_ftb_la_SOURCES = notify_ftb.c
notify_ftb_la_LDFLAGS = $(SO_LDFLAGS) $(PLUGIN_FLAGS)
