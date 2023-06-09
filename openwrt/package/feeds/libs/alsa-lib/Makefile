#
# Copyright (C) 2006-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=alsa-lib
PKG_VERSION:=1.1.8
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=ftp://ftp.alsa-project.org/pub/lib/ \
		http://distfiles.gentoo.org/distfiles/

PKG_HASH:=3cdc3a93a6427a26d8efab4ada2152e64dd89140d981f6ffa003e85be707aedf
PKG_MAINTAINER:=Ted Hess <thess@kitschensync.net>, \
		Peter Wagner <tripolar@gmx.at>

PKG_LICENSE:=LGPLv2.1 GPLv2
PKG_LICENSE_FILES:=COPYING aserver/COPYING

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1
PKG_USE_MIPS16:=0
PKG_CHECK_FORMAT_SECURITY:=0

include $(INCLUDE_DIR)/package.mk

define Package/alsa-lib
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=ALSA (Advanced Linux Sound Architecture) library
  URL:=http://www.alsa-project.org/
  DEPENDS:=@AUDIO_SUPPORT +libpthread +librt
endef

define Package/alsa-lib/description
 This is the library package for alsa, needed by some userspace programs.
 You must have enabled the ALSA support in the kernel.
endef

define Package/alsa-lib/conffiles
/etc/asound.conf
endef

define Package/alsa-lib/config
	source "$(SOURCE)/Config.in"
endef

PKG_CONFIG_DEPENDS := \
	CONFIG_ALSA_LIB_EXTRA_resmgr \
	CONFIG_ALSA_LIB_EXTRA_aload \
	CONFIG_ALSA_LIB_EXTRA_mixer \
	CONFIG_ALSA_LIB_EXTRA_pcm \
	CONFIG_ALSA_LIB_EXTRA_rawmidi \
	CONFIG_ALSA_LIB_EXTRA_hwdep \
	CONFIG_ALSA_LIB_EXTRA_seq \
	CONFIG_ALSA_LIB_EXTRA_ucm \
	CONFIG_ALSA_LIB_EXTRA_topology \
	CONFIG_ALSA_LIB_EXTRA_alisp

TARGET_CFLAGS += $(FPIC)
TARGET_LDFLAGS+=$(if $(CONFIG_USE_GLIBC),-ldl)

CONFIGURE_ARGS+= \
		--disable-python \
		--disable-debug \
		--without-debug \
		$(if $(CONFIG_ALSA_LIB_EXTRA_resmgr),--enable-resmgr,--disable-resmgr) \
		$(if $(CONFIG_ALSA_LIB_EXTRA_aload),--enable-aload,--disable-aload) \
		$(if $(CONFIG_ALSA_LIB_EXTRA_mixer),--enable-mixer,--disable-mixer) \
		$(if $(CONFIG_ALSA_LIB_EXTRA_pcm),--enable-pcm,--disable-pcm) \
		$(if $(CONFIG_ALSA_LIB_EXTRA_rawmidi),--enable-rawmidi,--disable-rawmidi) \
		$(if $(CONFIG_ALSA_LIB_EXTRA_hwdep),--enable-hwdep,--disable-hwdep) \
		$(if $(CONFIG_ALSA_LIB_EXTRA_seq),--enable-seq,--disable-seq) \
		$(if $(CONFIG_ALSA_LIB_EXTRA_ucm),--enable-ucm,--disable-ucm) \
		$(if $(CONFIG_ALSA_LIB_EXTRA_topology),--enable-topology,--disable-topology) \
		$(if $(CONFIG_ALSA_LIB_EXTRA_alisp),--enable-alisp,--disable-alisp) \
		$(SOFT_FLOAT_CONFIG_OPTION) \
		--with-versioned=no

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/alsa \
		$(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libasound.{la,so*} \
		$(1)/usr/lib/
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/alsa.pc \
		$(1)/usr/lib/pkgconfig/

	$(INSTALL_DIR) $(1)/usr/share/aclocal
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/share/aclocal/alsa.m4 \
		$(1)/usr/share/aclocal/
endef

define Package/alsa-lib/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libasound.so.* \
		$(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/share/alsa/{cards,pcm}
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/share/alsa/alsa.conf \
		$(1)/usr/share/alsa/
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/share/alsa/pcm/* \
		$(1)/usr/share/alsa/pcm/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/share/alsa/cards/* \
		$(1)/usr/share/alsa/cards/
endef

$(eval $(call BuildPackage,alsa-lib))
