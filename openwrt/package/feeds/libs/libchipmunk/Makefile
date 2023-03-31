#
# Copyright (C) 2007-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libchipmunk
PKG_VERSION:=5.3.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://chipmunk-physics.net/
PKG_MD5SUM:=556bce6fd18c93c6e0ca1a4a5dc44c36

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

PKG_FIXUP:=patch-libtool

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/${PKG_NAME}
  SECTION:=libs
  CATEGORY:=Libraries
  #DEPENDS:= +libc
  TITLE:=libchipmunk
  URL:=http://www.minigui.org/
endef

define Package/${PKG_NAME}/description
  The minigui mdolphin test case
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	(cd $(PKG_BUILD_DIR) && cmake $(PKG_BUILD_DIR))
endef

CONFIGURE_ARGS += \

LDFLAGS+=-L$(STAGING_DIR)/usr/lib/
define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/chipmunk/
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) \
		$(PKG_BUILD_DIR)/include/chipmunk \
		$(1)/usr/include/
	$(CP) \
                $(PKG_BUILD_DIR)/src/*.{a,so*} \
                $(1)/usr/lib/
endef
define Package/${PKG_NAME}/install
	$(INSTALL_DIR) $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/include/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/local/lib/*.{a,so*} \
		$(1)/usr/lib/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/local/include/chipmunk \
		$(1)/usr/include/chipmunk/
endef
$(eval $(call BuildPackage,${PKG_NAME}))
