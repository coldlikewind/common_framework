include $(top_srcdir)/build/modmake.rulesam

MODNAME=mod_ipsec
mod_LTLIBRARIES = mod_ipsec.la
mod_ipsec_la_SOURCES  = vici_api.c secgw_db_api.c secgw_config_api.c mod_ipsec.c
mod_ipsec_la_CFLAGS   = $(AM_CFLAGS)
mod_ipsec_la_LIBADD   = $(agc_builddir)/libagc.la
mod_ipsec_la_LDFLAGS  = -avoid-version -module -no-undefined -shared $(AGC_AM_LDFLAGS) -L/usr/local/strongswan/lib/ipsec/ -lvici

