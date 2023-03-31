include $(TOPDIR)/rules.mk

PKG_NAME:=libuapi
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

KERNEL_VERSION = CONF_KERNEL_VERSION_3_10

#config the kernel version,which depends on $(KERNEL_PATCHVER)
ifeq ($(KERNEL_PATCHVER),3.4)
	KERNEL_VERSION = CONF_KERNEL_VERSION_3_4
endif

ifeq ($(KERNEL_PATCHVER),3.10)
	KERNEL_VERSION = CONF_KERNEL_VERSION_3_10
endif

ifeq ($(KERNEL_PATCHVER),4.4)
	KERNEL_VERSION = CONF_KERNEL_VERSION_4_4
endif

ifeq ($(KERNEL_PATCHVER),4.9)
	KERNEL_VERSION = CONF_KERNEL_VERSION_4_9
endif

ifeq ($(KERNEL_PATCHVER),5.4)
	KERNEL_VERSION = CONF_KERNEL_VERSION_5_4
endif
#end config the kernel version

define Package/$(PKG_NAME)
  SUBMENU:=Display
  SECTION:=allwinner
  CATEGORY:=Allwinner
  TITLE:=display unit api for Allwinner
  MENU:=1
  DEPENDS:=+SUNXI_DISPLAY_GPU:libdrm +SUNXI_DISPLAY_GPU:libgpu
endef

define Package/$(PKG_NAME)/Default
endef

define Package/$(PKG_NAME)/description
  unit api for allwinner
endef

define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef

define Build/Prepare
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/
	$(CP) ./src $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

ifeq ($(CONFIG_SUNXI_DISPLAY_TWO),y)
TARGET_CFLAGS+=-D__SUNXI_DISPLAY2__
TARGET_CFLAGS+=-D$(KERNEL_VERSION)
endif
ifeq ($(CONFIG_SUNXI_DISPLAY_ONE),y)
TARGET_CFLAGS+=-D__SUNXI_DISPLAY__
TARGET_CFLAGS+=-D$(KERNEL_VERSION)
endif

ifeq ($(CONFIG_SUNXI_ALLOC_CMA),y)
TARGET_CFLAGS+=-DCONF_KERNEL_CMA
endif
ifeq ($(CONFIG_SUNXI_ALLOC_IOMMU),y)
TARGET_CFLAGS+=-DCONF_KERNEL_IOMMU
endif

ifeq ($(CONFIG_LUAPI_LAYER_ALLOC_CHN),y)
TARGET_CFLAGS+=-DLUAPI_LAYER_ALLOC_CHN
endif
ifeq ($(CONFIG_LUAPI_LAYER_ALLOC_LAY),y)
TARGET_CFLAGS+=-DLUAPI_LAYER_ALLOC_LAY
endif

ifeq ($(CONFIG_SUNXI_DISPLAY_GPU),y)
	#TARGET_CFLAGS+=-DSUNXI_DISPLAY_GPU
	TARGET_CFLAGS+=-DSUNXI_DISPLAY_GPU -DSUNXI_DISPLAY_GPU_BUF
	TARGET_LDFLAGS+=-lEGL -lMali -lGLESv2
else
	TPLAYER_EXTRA_CFLAGS+=-DSUNXI_DISPLAY_DE
endif

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src\
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		INSTALL_PREFIX="$(PKG_INSTALL_DIR)" \
		all
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/include

	$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
