include $(TOPDIR)/rules.mk

PKG_NAME:=wifimanager
PKG_VERSION:=2.1
PKG_RELEASE:=1

SRC_CODE_DIR := $(LICHEE_CBBPKG_DIR)/allwinner/wireless/wifimanager

PKG_CONFIG_DEPENDS += CONFIG_WMG_PROTOCOL_SOFTAP CONFIG_WMG_PROTOCOL_BLE CONFIG_WMG_PROTOCOL_XCONFIG CONFIG_WMG_PROTOCOL_SOUNDWAVE

PKG_FILE_DEPENDS += $(SRC_CODE_DIR)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)/Default
  SUBMENU:=Wireless
  SECTION:=allwinner
  CATEGORY:=Allwinner
  TITLE:=Tina wifimanager
  DEPENDS:=+libpthread +libnl-tiny +libstdcpp +wirelesscommon +libopenssl
endef

define Package/$(PKG_NAME)
  $(call Package/wifimanager/Default)
  MENU:=1
  DEFAULT:=1
endef

ifdef CONFIG_WMG_PROTOCOL_BLE
DEMO_DEPENDS_FILE += +btmanager +libjson-c
endif

ifdef CONFIG_WMG_PROTOCOL_XCONFIG
DEMO_DEPENDS_FILE += +libopenssl
endif

ifdef CONFIG_WMG_PROTOCOL_SOUNDWAVE
DEMO_DEPENDS_FILE += +alsa-lib +libuci
endif

define Package/$(PKG_NAME)-demo
  $(call Package/wifimanager/Default)
  TITLE:=Tina wifimanager app demo
  DEPENDS := wifimanager +wifimanager +wirelesscommon $(DEMO_DEPENDS_FILE)
endef


define Package/$(PKG_NAME)-demo/config
source "$(SOURCE)/Config.in"
endef

PKG_CONFIG_DEPENDS += CONFIG_WIFIMANAGER_ENABLE_DAEMON

define Package/$(PKG_NAME)/description
	Tina wifi manager
endef

define Package/$(PKG_NAME)-demo/description
	Tina wifimanager app demo
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) -r $(SRC_CODE_DIR)/core $(PKG_BUILD_DIR)/
	$(CP) -r $(SRC_CODE_DIR)/demo $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

TARGET_CFLAGS += $(if $(CONFIG_USE_GLIBC),-pthread)
TARGET_LDFLAGS += $(if $(CONFIG_USE_GLIBC),-lpthread -lrt -ldl)
##TARGET_CFLAGS += -DOS_NET_LINUX_OS

define Build/Compile/$(PKG_NAME)
	#libwifimg.so
	$(MAKE) -C $(PKG_BUILD_DIR)/core/ \
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		CONFIG_PREFIX="$(PKG_INSTALL_DIR)" \
		all
endef

ifeq ($(CONFIG_PACKAGE_wifimanager-demo),y)
define Build/Compile/$(PKG_NAME)-demo
	$(MAKE) -C $(PKG_BUILD_DIR)/demo/ \
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		CONFIG_PREFIX="$(PKG_INSTALL_DIR)" \
		all
endef
endif

define Build/Compile
	$(Build/Compile/$(PKG_NAME))
	$(Build/Compile/$(PKG_NAME)-demo)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/libwifimg.so $(1)/lib
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/include/*.h $(1)/usr/include
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DIR) $(1)/etc/wifi
	$(INSTALL_DIR) $(1)/etc/wifi/wpa_supplicant
	$(INSTALL_DIR) $(1)/etc/wifi/hostapd
	$(INSTALL_DIR) $(1)/etc/init.d

	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/libwifimg.so $(1)/lib
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/include/*.h $(1)/usr/include
	$(INSTALL_DATA) $(SRC_CODE_DIR)/files/hostapd.conf $(1)/etc/wifi/hostapd
	$(INSTALL_DATA) $(SRC_CODE_DIR)/files/wpa_supplicant.conf $(1)/etc/wifi/wpa_supplicant
	$(INSTALL_DATA) $(SRC_CODE_DIR)/files/wpa_supplicant_src.conf $(1)/etc/wifi/wpa_supplicant
	$(INSTALL_DATA) $(SRC_CODE_DIR)/files/wpa_supplicant_overlay.conf $(1)/etc/wifi/wpa_supplicant
	$(INSTALL_DATA) $(SRC_CODE_DIR)/files/wifi_monitor.conf $(1)/etc/wifi
	$(INSTALL_BIN) $(SRC_CODE_DIR)/files/wpa_supplicant.init $(1)/etc/init.d/wpa_supplicant
	$(INSTALL_BIN) $(SRC_CODE_DIR)/files/udhcpc_wlan0.init $(1)/etc/wifi/udhcpc_wlan0
endef

define Package/$(PKG_NAME)-demo/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_DIR) $(1)/etc/wifi
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/wifi_daemon $(1)/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/wifi $(1)/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,$(PKG_NAME)-demo))
