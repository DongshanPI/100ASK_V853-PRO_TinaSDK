#
# Copyright (C) 2007-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=qt-easing
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
HAVE_SOURCE := yes

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=Qt
  SUBMENU:=Qt
  CATEGORY:=Gui
  DEPENDS:= +qt5-widgets +qt5-gui +qt5-core
  TITLE:=qt-easing
  MAINTAINER:=anruliu<anruliu@allwinnertech.com>
endef

define Package/$(PKG_NAME)/description
  The qt easing application for Demo
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/
	$(CP) -r ./src/* $(PKG_BUILD_DIR)/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/qt-easing $(1)/usr/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
