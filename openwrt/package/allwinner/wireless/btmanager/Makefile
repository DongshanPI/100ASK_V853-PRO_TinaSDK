include $(TOPDIR)/rules.mk

PKG_NAME:=btmanager
PKG_VERSION:=4.0.0
PKG_RELEASE:=1
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

SRC_CODE_DIR := $(TOPDIR)/../../platform/allwinner/wireless/btmanager

PKG_CONFIG_DEPENDS:= \
	CONFIG_USES_BCMDHD \
	CONFIG_USES_XR829 \
	CONFIG_USES_XR819S \
	CONFIG_USES_REALTEK \
	CONFIG_USES_ESP8089

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)/Default
  SECTION:=utils
  CATEGORY:=Allwinner
  SUBMENU:=Wireless
endef

define Package/$(PKG_NAME)
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=bluetooth manager core
  DEPENDS:=+alsa-lib +bluez-daemon +glib2 +sbc +dbus +bluez-alsa +libjson-c +ell +wirelesscommon
  KCONFIG:=\
	  CONFIG_HFP_OVER_PCM=y
endef

define Package/$(PKG_NAME)/config
config BTMG_ENABLE_DEMO
	bool "Enable btmanager demo support"
	depends on PACKAGE_btmanager
	default y

config BTMG_ENABLE_MESH
	bool "Enable xradio mesh support"
	depends on PACKAGE_btmanager
	default n
endef

define Package/$(PKG_NAME)/description
	bluetooth manager
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) -r $(SRC_CODE_DIR)/src $(PKG_BUILD_DIR)/
	$(CP) -r $(SRC_CODE_DIR)/demo $(PKG_BUILD_DIR)/
endef

TARGET_LDFLAGS += $(if $(CONFIG_USE_GLIBC),-lpthread -lz -lrt -lm -ldl)

PKG_CONFIG_DEPENDS += CONFIG_BTMG_ENABLE_MESH
PKG_CONFIG_DEPENDS += CONFIG_BTMG_ENABLE_DEMO
define Build/Configure
endef
ifeq ($(CONFIG_PACKAGE_$(PKG_NAME)),y)
define Build/Compile/$(PKG_NAME)
	$(MAKE) -C $(PKG_BUILD_DIR)/src/ \
		PKG_BUILD_DIR="$(PKG_BUILD_DIR)" \
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		BT_EXT_CFLAG="$(BT_EXT_CFLAG)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		STAGING_DIR="$(STAGING_DIR)" \
		CONFIG_LIBC="$(CONFIG_LIBC)$(if $(CONFIG_EXT_TOOLCHAIN_GCC_8_3_0),-gcc8_3_0)" \
		CPU="$(ARCH)" \
		CONFIG_PREFIX="$(PKG_INSTALL_DIR)" \
		all
ifeq ($(CONFIG_BTMG_ENABLE_DEMO), y)
	$(MAKE) -C $(PKG_BUILD_DIR)/demo/ \
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		STAGING_DIR="$(STAGING_DIR)" \
		CONFIG_PREFIX="$(PKG_INSTALL_DIR)" \
		all
endif
ifeq ($(CONFIG_BTMG_ENABLE_MESH), y)
	$(MAKE) -C $(PKG_BUILD_DIR)/src/mesh \
		PKG_BUILD_DIR="$(PKG_BUILD_DIR)" \
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		BT_EXT_CFLAG="$(BT_EXT_CFLAG)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		STAGING_DIR="$(STAGING_DIR)" \
		CONFIG_LIBC="$(CONFIG_LIBC)$(if $(CONFIG_EXT_TOOLCHAIN_GCC_8_3_0),-gcc8_3_0)" \
		CPU="$(ARCH)" \
		CONFIG_PREFIX="$(PKG_INSTALL_DIR)" \
		all
	$(MAKE) -C $(PKG_BUILD_DIR)/demo/mesh/ \
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		CONFIG_PREFIX="$(PKG_INSTALL_DIR)" \
		all
endif
endef
endif

define Build/Compile
	$(Build/Compile/$(PKG_NAME))
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/include/*.h $(1)/usr/include
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/lib
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/lib
	$(INSTALL_DIR) $(1)/etc/bluetooth
	cp $(SRC_CODE_DIR)/config/bt_init.sh $(SRC_CODE_DIR)/config/bt_init.sh.tmp
ifeq ($(CONFIG_USES_XR819S), y)
	sed -i 's/xradio/xr819s/g' $(SRC_CODE_DIR)/config/bt_init.sh.tmp
endif
ifeq ($(CONFIG_USES_REALTEK), y)
	sed -i 's/bt_hciattach="hciattach"/bt_hciattach="rtk_hciattach"/g' $(SRC_CODE_DIR)/config/bt_init.sh.tmp
	sed -i 's/# reset_bluetooth_power/reset_bluetooth_power/g' $(SRC_CODE_DIR)/config/bt_init.sh.tmp
	sed -i 's/ttyS1 xradio/-s 115200 \/dev\/ttyS1 rtk_h5/g' $(SRC_CODE_DIR)/config/bt_init.sh.tmp
endif
ifneq (,$(findstring UART2, $(BT_UART_INDEX)))
	sed -i 's/ttyS1/ttyS2/g' $(SRC_CODE_DIR)/config/bt_init.sh.tmp
endif
	$(INSTALL_BIN) $(SRC_CODE_DIR)/config/bt_init.sh.tmp $(1)/etc/bluetooth/bt_init.sh
	rm -rf $(SRC_CODE_DIR)/config/bt_init.sh.tmp
	$(INSTALL_DATA) $(SRC_CODE_DIR)/config/bluetooth.json $(1)/etc/bluetooth/bluetooth.json

ifeq ($(CONFIG_BTMG_ENABLE_DEMO), y)
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/bt_test $(1)/usr/bin
endif

ifeq ($(CONFIG_BTMG_ENABLE_MESH), y)
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/bt_mesh_test $(1)/usr/bin
endif
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
