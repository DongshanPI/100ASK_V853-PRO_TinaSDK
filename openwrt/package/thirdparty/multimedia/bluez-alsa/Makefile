include $(TOPDIR)/rules.mk

PKG_NAME:=bluez-alsa
PKG_VERSION:=20211122
PKG_RELEASE:=1
PKG_MAINTAINER:=
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE.txt

PKG_SOURCE:=bluez-alsa-$(PKG_VERSION).tar.gz

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk


define Package/$(PKG_NAME)
  SECTION:=multimedia
  CATEGORY:=Multimedia
  DEPENDS:=+alsa-lib +bluez-daemon +glib2 +sbc +fdk-aac
  TITLE:=Bluetooth Audio ALSA backend
  URL:=https://github.com/Arkq/bluez-alsa
endef

define Package/$(PKG_NAME)/description
  ALSA plugin supporting Bluetooth audio bluez-alsa
endef

#CONFIGURE_ARGS += --enable-aac --enable-ofono --enable-debug
#CONFIGURE_ARGS += --enable-debug
TARGET_LDFLAGS += $(if $(CONFIG_USE_GLIBC), -ldl -lresolv)

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/share/alsa/alsa.conf.d/
	$(CP) $(PKG_BUILD_DIR)/src/asound/20-bluealsa.conf $(1)/usr/share/alsa/alsa.conf.d/
	$(INSTALL_DIR) $(1)/usr/lib/alsa-lib
	$(CP) $(PKG_BUILD_DIR)/src/asound/.libs/libasound_module_*_bluealsa.so $(1)/usr/lib/alsa-lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/bluealsa $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/utils/aplay/bluealsa-aplay $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/dbus-1/system.d/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/src/bluealsa.conf $(1)/etc/dbus-1/system.d/bluealsa.conf
endef

$(eval $(call BuildPackage,bluez-alsa))
