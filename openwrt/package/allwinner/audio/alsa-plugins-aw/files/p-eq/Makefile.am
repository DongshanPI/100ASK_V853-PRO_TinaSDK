asound_module_pcm_awequal_LTLIBRARIES = libasound_module_pcm_awequal.la

asound_module_pcm_awequaldir = @ALSA_PLUGIN_DIR@

AM_CFLAGS = -Wall -g @ALSA_CFLAGS@ -I./
AM_LDFLAGS = -module -avoid-version -export-dynamic -Wl,--no-undefined $(LDFLAGS_NOUNDEFINED) \
			 -Wl,--whole-archive libs/$(host)/libeq.a -Wl,--no-whole-archive \
			 -lm -laactd

libasound_module_pcm_awequal_la_SOURCES = pcm_awequal.c

uninstall-hook:
	rm -f $(DESTDIR)$(libdir)/alsa-lib/libasound_module_pcm_awequal.so

noinst_HEADERS = eq.h
