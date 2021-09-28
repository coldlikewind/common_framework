include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_sysinfo

mod_LTLIBRARIES = mod_sysinfo.la
mod_sysinfo_la_SOURCES  =  mod_sysinfo.cc SysInfo.cc SysAlarm.cc cfgNetwork.cc SysInfoDB.cc
mod_sysinfo_la_CPPFLAGS = $(AM_CXXFLAGS)
mod_sysinfo_la_LIBADD   = $(agc_builddir)/libagc.la
mod_sysinfo_la_LDFLAGS  = -avoid-version -module -no-undefined -shared  $(AGC_AM_LDFLAGS)
