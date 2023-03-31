include $(TOPDIR)/rules.mk

PKG_NAME:=libatomic_ops
PKG_VERSION:=7.6.8
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://github.com/ivmai/libatomic_ops
PKG_MD5SUM:=8298aa5b2e7ffef7490f427fafb3363a

#PKG_SOURCE_PROTO:=git
#PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
#PKG_SOURCE_VERSION:=b68c654614b5dbec5b71d0250e42e8a3a8b1eda7

PKG_LICENSE_FILES:=doc/LICENSING.txt

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=The atomic_ops library
endef

define Package/$(PKG_NAME)/description
  This package provides semi-portable access to hardware-provided atomic memory
  update operations on a number architectures.
endef

define Build/Prepare
	$(Build/Prepare/Default)
	(cd $(PKG_BUILD_DIR) && ./autogen.sh)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/usr/lib/
endef

define Package/$(PKG_NAME)/install
	:
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
