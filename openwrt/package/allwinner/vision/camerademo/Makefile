include $(TOPDIR)/rules.mk

PKG_NAME:=camerademo
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)/config
	if PACKAGE_$(PKG_NAME)
		config ENABLE_VIN_ISP
			bool "Enabel vin isp support"
			default n
	endif
endef

PKG_CONFIG_DEPENDS += CONFIG_ENABLE_VIN_ISP

define Package/$(PKG_NAME)
  SUBMENU:=Vision
  SECTION:=allwinner
  CATEGORY:=Allwinner
  MENU:=1
  TITLE:=camerademo test sensor
  DEPENDS:=+ENABLE_VIN_ISP:libAWIspApi
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) -r ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/ \
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		USE_VIN_ISP="$(CONFIG_ENABLE_VIN_ISP)" \
		all
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) 	$(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
