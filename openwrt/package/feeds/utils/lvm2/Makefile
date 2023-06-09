#
# Copyright (C) 2009-2010 Stefan Monnier
# Copyright (C) 2011-2018 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v3+.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=LVM2
PKG_VERSION:=2.03.11
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME).$(PKG_VERSION).tgz
PKG_SOURCE_URL:=https://sourceware.org/pub/lvm2
PKG_HASH:=842c4510d4653990927d4518a5bf2743126a37531671a05842cdaf8d54bb9dd4
PKG_BUILD_DIR:=$(BUILD_DIR)/lvm2-$(BUILD_VARIANT)/$(PKG_NAME).$(PKG_VERSION)
HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/lvm2-$(BUILD_VARIANT)/$(PKG_NAME).$(PKG_VERSION)

PKG_MAINTAINER:=Daniel Golle <daniel@makrotopia.org>
PKG_LICENSE:=GPL-2.0 LGPL-2.1
PKG_LICENSE_FILES:=COPYING COPYING.lib
PKG_CPE_ID:=cpe:/a:heinz_mauelshagen:lvm2

PKG_BUILD_PARALLEL:=1
HOST_BUILD_PARALLEL:=1

HOST_BUILD_DEPENDS:=libaio/host

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/libdevmapper/Default
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=The Linux Kernel Device Mapper userspace library
  URL:=https://sourceware.org/dm/
  DEPENDS:=+libpthread +libuuid +librt +libblkid
endef


define Package/libdevmapper-normal
  $(call Package/libdevmapper/Default)
  VARIANT := normal
  PROVIDES := libdevmapper
endef

define Package/libdevmapper-selinux
  $(call Package/libdevmapper/Default)
  VARIANT := selinux
  DEPENDS += +libselinux
  PROVIDES := libdevmapper
endef

define Package/libdevmapper-normal/description
 The device-mapper is a component of the 2.6 linux kernel that supports logical
 volume management. It is required by LVM2 and EVMS.
endef

define Package/libdevmapper-selinux/description
$(call Package/libdevmapper-normal/description)
 This variant supports SELinux

endef

define Package/lvm2/default
  SECTION:=utils
  CATEGORY:=Utilities
  SUBMENU:=Disc
  TITLE:=The Linux Logical Volume Manager
  URL:=https://sourceware.org/lvm2/
  DEPENDS:=+libreadline +libncurses +libaio
endef

define Package/lvm2-normal
  $(call Package/lvm2/default)
  VARIANT := normal
  DEPENDS += +libdevmapper-normal
  PROVIDES := lvm2
endef

define Package/lvm2-selinux
  $(call Package/lvm2/default)
  VARIANT := selinux
  DEPENDS += +libdevmapper-selinux
  PROVIDES := lvm2
endef

define Package/lvm2-normal/description
 LVM2 refers to a new userspace toolset that provide logical volume management
 facilities on linux. It is reasonably backwards-compatible with the original
 LVM toolset.
endef

define Package/lvm2-selinux/description
$(call Package/lvm2-normal/description)
 This variant supports SELinux

endef


CONFIGURE_ARGS += \
	--disable-o_direct \
	--with-default-pid-dir=/var/run \
	--with-default-dm-run-dir=/var/run \
	--with-default-run-dir=/var/run/lvm \
	--with-default-locking-dir=/var/lock/lvm \
	--$(if $(findstring selinux,$(BUILD_VARIANT)),en,dis)able-selinux

ifneq ($(shell /bin/sh -c "echo -n 'X'"),X)
MAKE_SHELL = SHELL=/bin/bash
endif

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) $(FPIC)" \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		$(MAKE_SHELL) \
		install
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/include/libdevmapper.h $(1)/usr/include
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libdevmapper.so* $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_BUILD_DIR)/libdm/libdevmapper.pc $(1)/usr/lib/pkgconfig
endef

define Package/libdevmapper-normal/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libdevmapper.so.* $(1)/usr/lib
endef

Package/libdevmapper-selinux/install = $(Package/libdevmapper-normal/install)

define Package/lvm2-normal/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/lvm $(1)/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/dmsetup $(1)/sbin
	$(INSTALL_DIR) $(1)/etc/lvm
	$(SED) '/^[[:space:]]*\(#\|$$$$\)/d; /cache_dir/s@.*@	cache_dir = "/tmp/lvm/cache"@' $(PKG_INSTALL_DIR)/etc/lvm/lvm.conf
	$(INSTALL_CONF) $(PKG_INSTALL_DIR)/etc/lvm/lvm.conf $(1)/etc/lvm/
	$(INSTALL_CONF) $(PKG_INSTALL_DIR)/etc/lvm/lvmlocal.conf $(1)/etc/lvm/
	$(INSTALL_DIR) $(1)/etc/lvm/profile
	$(INSTALL_CONF) $(PKG_INSTALL_DIR)/etc/lvm/profile/* $(1)/etc/lvm/profile/
	$(INSTALL_DIR) $(1)/lib/preinit
	$(INSTALL_DATA) ./files/lvm2.preinit $(1)/lib/preinit/80_lvm2
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/lvm2.init $(1)/etc/init.d/lvm2
	$(FIND) $(PKG_INSTALL_DIR)/usr/sbin/ -type l -exec $(CP) -a {} $(1)/sbin/ \;
endef

Package/lvm2-selinux/install = $(Package/lvm2-normal/install)

define Package/lvm2-normal/conffiles
/etc/lvm/lvm.conf
endef

Package/lvm2-selinux/conffiles = $(Package/lvm2/conffiles)


HOST_CONFIGURE_ARGS += --disable-o_direct

define Host/Install
	$(MAKE) -C $(HOST_BUILD_DIR)/libdm install
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,libdevmapper-normal))
$(eval $(call BuildPackage,libdevmapper-selinux))
$(eval $(call BuildPackage,lvm2-normal))
$(eval $(call BuildPackage,lvm2-selinux))
