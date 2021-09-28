include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_sig

mod_LTLIBRARIES = mod_sig.la
mod_sig_la_SOURCES  =  mod_sig.cc \
	ApmManager.cc AsmManager.cc CfgHelper.cc EpmManager.cc EsmManager.cc \
	NgapProtocol.cc NgapSessManager.cc NsmManager.cc PduSessManager.cc \
	PlmnUtil.cc RanNodeId.cc SctpLayer.cc Tai.cc SigApi.cc DbManager.cc sig_media_path.cc SigAlarm.cc
mod_sig_la_CPPFLAGS = $(AM_CXXFLAGS) $(NGAPCODEC_CFLAGS)
mod_sig_la_LIBADD   = $(agc_builddir)/libagc.la
mod_sig_la_LDFLAGS  = -avoid-version -module -no-undefined -shared  $(NGAPCODEC_LIBS)
