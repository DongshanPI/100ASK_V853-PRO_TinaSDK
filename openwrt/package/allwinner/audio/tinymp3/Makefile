include $(TOPDIR)/rules.mk

# Name and release number of this package
PKG_NAME:=tinymp3
PKG_VERSION:=1
PKG_RELEASE:=1


PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
HAVE_SOURCE := yes

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)/Default
  SUBMENU:=Audio
  SECTION:=allwinner
  CATEGORY:=Allwinner
  DEPENDS:=+libpthread +libmad +alsa-lib
endef

define Package/$(PKG_NAME)
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=tiny mp3 player
endef

define Package/$(PKG_NAME)/description
endef

TARGET_LDFLAGS+=$(if $(CONFIG_USE_GLIBC),-ldl -lm)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/
	$(CP) -r ./src $(PKG_BUILD_DIR)/
endef

$(warning "ARCH=$(ARCH), LIBC=$(LIBC)")

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		MAKE="$(MAKE)" \
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		CFLAGS="$(TARGET_CFLAGS)"\
		LDFLAGS="$(TARGET_LDFLAGS)" \
		CONFIG_PREFIX="$(PKG_INSTALL_DIR)" \
		C_LIB_TYPE="$(LIBC)" \
		KERNEL64_USER32="$(CONFIG_COMPLILE_KERNEL64_USER32)" \
		all
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/lib/*.so $(1)/lib/
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/include/*.h $(1)/usr/include/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/lib/* $(1)/lib/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/bin/* $(1)/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
