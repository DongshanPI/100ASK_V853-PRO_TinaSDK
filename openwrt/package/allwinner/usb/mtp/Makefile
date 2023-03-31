include $(TOPDIR)/rules.mk

PKG_NAME:=MtpDaemon
PKG_VERSION:=1
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

SRC_CODE_DIR := $(LICHEE_CBBPKG_DIR)/allwinner/usb/mtp/src

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)/Default
  SUBMENU:=USB
  SECTION:=allwinner
  CATEGORY:=Allwinner
  DEPENDS:=+libpthread
endef

define Package/$(PKG_NAME)
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=Tina MTP Daemon
endef

PKG_CONFIG_DEPENDS += CONFIG_MTP_USE_AIO
PKG_FILE_DEPENDS := $(SRC_CODE_DIR)

define Package/$(PKG_NAME)/description
  tina MTP service
endef

define Package/$(PKG_NAME)/config
if PACKAGE_MtpDaemon
config MTP_USE_AIO
    depends on KERNEL_AIO
    bool
    prompt "MTP daemon compile with aio"
    default y
endif
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(SRC_CODE_DIR) $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS)"\
		LDFLAGS="$(TARGET_LDFLAGS)" \
		USE_AIO=$(CONFIG_MTP_USE_AIO) \
		all
endef

define Build/InstallDev
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/MtpDaemon $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/MtpTools $(1)/usr/bin

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./mtpdaemon.init $(1)/etc/init.d/mtp
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
