include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_media

mod_LTLIBRARIES = mod_media.la
mod_media_la_SOURCES  =  mod_media.c \
	agc_gtp_path.c media_context.c media_gtp_path.c media_sig_path.c media_tun_path.c media_db.c
mod_media_la_CFLAGS = $(AM_CFLAGS)
mod_media_la_LIBADD   = $(agc_builddir)/libagc.la
mod_media_la_LDFLAGS  = -avoid-version -module -no-undefined -shared 
