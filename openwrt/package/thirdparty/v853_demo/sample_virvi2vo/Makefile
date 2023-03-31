include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/package.mk

PKG_NAME:=sample_virvi2vo
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(LICHEE_PLATFORM_DIR)/$(PKG_NAME)
SRC_CODE_DIR := ./bin

define Package/$(PKG_NAME)
  SECTION:=v853_demo
  CATEGORY:=Demo for v853 board
  DEPENDS:=+libc +libstdcpp
  TITLE:=sample_virvi2vo binary for v853 eval
endef

define Package/$(PKG_NAME)/extra_provides
	echo 'libasound.so.2' && echo 'libstdc++.so.6'
endef

define Package/$(PKG_NAME)/config
endef

define Package/$(PKG_NAME)/Default
endef

define Package/$(PKG_NAME)/description
  sample_virvi2vo binary for v853 eval
endef

define Build/Prepare
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/
	$(CP) -rf $(SRC_CODE_DIR)/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
endef

#define Build/InstallDev
#	$(INSTALL_DIR) $(1)/usr/lib
#endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sample_virvi2vo $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lib/lib* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
