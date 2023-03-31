#
# Copyright (C) 2019 Lucian Cristian
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=nspr
PKG_VERSION:=4.12
PKG_RELEASE:=1
PKG_MAINTAINER:=Lucian Cristian <lucian.cristian@gmail.com>
PKG_LICENCE:=MPL-2.0

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:= \
    https://download.cdn.mozilla.net/pub/$(PKG_NAME)/releases/v$(PKG_VERSION)/src/ \
    https://archive.mozilla.org/pub/$(PKG_NAME)/releases/v$(PKG_VERSION)/src/
PKG_HASH:=e0b10a1e569153668ff8bdea6c7e491b389fab69c2f18285a1ebf7c2ea4269de

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1
PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

CONFIGURE_PATH = ./nspr
MAKE_PATH = ./nspr
PKG_AUTOMAKE_PATHS = $(PKG_BUILD_DIR)/nspr
LBITS = $(shell $(TARGET_CC) -dM -E - </dev/null | grep -q "__LP64__" && echo 64 || echo 32)

ifeq ($(LBITS),64)
    conf=--enable-64bit
endif

export MUSL=$(if $(CONFIG_LIBC_USE_GLIBC),0,1)
TARGET_LDFLAGS += -Wl,--gc-sections,--as-needed

STAGING_DIR_HOST = $(STAGING_DIR)/../hostpkg

CONFIGURE_ARGS += \
    --build=$(GNU_HOST_NAME) \
    --host=$(GNU_HOST_NAME) \
    --target=$(REAL_GNU_TARGET_NAME) \
    --disable-debug \
    --with-pthreads \
    $(if $(CONFIG_IPV6),--enable-ipv6,--disable-ipv6) \
    $(conf)

define Host/Configure
	$(call Host/Configure/Default,--enable-64bit , ,nspr)
endef

define Host/Compile
	echo $(PKG_BUILD_DIR)
	$(MAKE) -C $(PKG_BUILD_DIR)/nspr/config
	$(MAKE) -C $(HOST_BUILD_DIR)/nspr
endef

define Host/Install
	$(MAKE) -C $(HOST_BUILD_DIR)/nspr install
endef

define Build/Compile
	CROSS_COMPILE=1 CFLAGS="-DXP_UNIX $(HOST_CFLAGS)" LDFLAGS="" CC="$(HOSTCC)" \
	    $(MAKE) -C $(PKG_BUILD_DIR)/nspr/config
	$(call Build/Compile/Default)
endef

define Package/nspr
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Netscape Portable Runtime (NSPR)
  URL:=https://developer.mozilla.org/en-US/docs/Mozilla/Projects/NSPR
  DEPENDS:=+libpthread
endef

define Package/nspr/description
  Netscape Portable Runtime (NSPR) provides a platform-neutral API for system
  level and libc-like functions. The API is used in the Mozilla clients, many
  of Red Hat's and Oracle's server applications, and other software offerings.
endef

define Build/InstallDev
	echo $(STAGING_DIR_HOST)+++++++++++++++++++++++++++
	echo $(PKG_BUILD_DIR)++++++++++++++++++++++++++++
	$(INSTALL_DIR) \
	    $(1)/usr/include \
	    $(1)/usr/lib/ \
	    $(1)/usr/lib/pkgconfig/ \
	    $(1)/usr/share/aclocal/

	$(CP) $(PKG_INSTALL_DIR)/usr/include/nspr $(1)/usr/include/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/* $(1)/usr/lib/pkgconfig/
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/aclocal/* $(1)/usr/share/aclocal/
endef

define Package/nspr/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,nspr))
$(eval $(call HostBuild))

# include $(TOPDIR)/rules.mk

# PKG_NAME:=nspr
# PKG_VERSION:=4.12
# PKG_RELEASE:=1
# PKG_MAINTAINER:=Lucian Cristian <lucian.cristian@gmail.com>
# PKG_LICENCE:=MPL-2.0

# PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
# PKG_SOURCE_URL:= \
#     https://download.cdn.mozilla.net/pub/$(PKG_NAME)/releases/v$(PKG_VERSION)/src/ \
#     https://archive.mozilla.org/pub/$(PKG_NAME)/releases/v$(PKG_VERSION)/src/
# PKG_HASH:=15ea32c7b100217b6e3193bc03e77f485d9bf7504051443ba9ce86d1c17c6b5a

# PKG_BUILD_PARALLEL:=1
# PKG_INSTALL:=1
# PKG_FIXUP:=autoreconf

# # include $(BUILD_DIR)/package.mk
# # include $(BUILD_DIR)/host-build.mk

# include $(INCLUDE_DIR)/package.mk
# include $(INCLUDE_DIR)/host-build.mk

# define Package/nspr
#   SECTION:=libs
#   CATEGORY:=Libraries
#   TITLE:=Netscape Portable Runtime (NSPR)
#   URL:=https://developer.mozilla.org/en-US/docs/Mozilla/Projects/NSPR
#   DEPENDS:=+libpthread
# endef

# # define Package/nspr
# #   SECTION:=libs
# #   CATEGORY:=Libraries
# #   TITLE:=A library for accessing Linux USB devices
# #   DEPENDS:=+libpthread +librt
# #   URL:=http://libusb.info/
# #   ABI_VERSION:=0
# # endef

# define Package/nspr/description
#   libusb is a C library that gives applications easy access to USB devices on
#   many different operating systems.
# endef

# TARGET_CFLAGS += $(FPIC)
# CONFIGURE_ARGS += \
# 	--disable-udev \
# 	--disable-log

# define Build/InstallDev
# 	$(CP) $(PKG_INSTALL_DIR)/* $(1)/
# endef

# define Package/nspr/install
# 	$(INSTALL_DIR) $(1)/usr/lib
# 	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libusb*.so.* $(1)/usr/lib/
# endef

# $(eval $(call BuildPackage,nspr))
