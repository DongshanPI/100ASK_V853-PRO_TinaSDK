#
# Copyright (C) 2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=g2d-sample
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=g2d-sample
  CATEGORY:=Allwinner
  SUBMENU:=Display
  TITLE:=g2d demo function
  DEPENDS:=@DISPLAY_SUPPORT +kmod-sunxi-g2d
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/g2d_bld_dmabuf $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/g2d_fill_dmabuf $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/g2d_lbc_rot $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/g2d_mixer_task $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
