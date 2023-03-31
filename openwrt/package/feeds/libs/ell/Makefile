include $(TOPDIR)/rules.mk

PKG_NAME:=ell
PKG_VERSION:=0.16
PKG_RELEASE:=1
PKG_MAINTAINER:=
PKG_LICENSE:=GPLv2
PKG_LICENSE_FILES:=COPYING

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://mirrors.edge.kernel.org/pub/linux/libs/ell/
PKG_HASH:=f328271df98828a8d6fba3a7f7f4f0c38d383ad8ea6d158d2fffe5a2296d5170

PKG_FIXUP:=autoreconf

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/ell
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Embedded Linux Library
  URL:=https://mirrors.edge.kernel.org/pub/linux/libs/ell/

endef

define Package/ell/description
  The Embedded Linux Library(ELL) provides core, low-level
functionality for system daemons.
endef

CONFIGURE_ARGS += --disable-glib

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/ell $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.{so*,la} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig/
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/* $(1)/usr/lib/pkgconfig/
endef

define Package/ell/install
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/ell/.libs/*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,ell))
