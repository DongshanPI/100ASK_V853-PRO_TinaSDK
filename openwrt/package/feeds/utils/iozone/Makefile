#
# Copyright (C) 2006-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=iozone3
PKG_VERSION:=489
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION).tar
PKG_SOURCE_URL:=http://www.iozone.org/src/current/
PKG_MD5SUM:=deda177b4ba4e7d05440b0d317549085
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)_$(PKG_VERSION)
PKG_LICENSE:=

include $(INCLUDE_DIR)/package.mk

define Package/iozone3
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Iozone Filesystem Benchmark
endef

define Package/iozone3/description
 iozone is a tool to test filesystem performance
endef

TARGET_CFLAGS +=

define Build/Configure
endef

define Build/Compile
	mkdir -p $(PKG_INSTALL_DIR)
	$(MAKE) -C $(PKG_BUILD_DIR)/src/current \
		CC="$(TARGET_CC)" \
		GCC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		linux
endef

define Package/iozone3/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/current/iozone $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/current/fileop $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/current/pit_server $(1)/usr/bin
endef

$(eval $(call BuildPackage,iozone3))
