#
# Copyright (C) 2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=yuview
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/yuview
  SUBMENU:=Display
  SECTION:=allwinner
  CATEGORY:=Allwinner
  TITLE:=Dispay show yuv frame
  DEPENDS:=@DISPLAY_SUPPORT +libuapi
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

ifeq ($(CONFIG_SUNXI_ALLOC_CMA),y)
TARGET_CFLAGS+=-DCONF_KERNEL_CMA
endif
ifeq ($(CONFIG_SUNXI_ALLOC_IOMMU),y)
TARGET_CFLAGS+=-DCONF_KERNEL_IOMMU
endif

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/yuview/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/yuview $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,yuview))
