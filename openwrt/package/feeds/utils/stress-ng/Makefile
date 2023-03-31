#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=stress-ng
PKG_VERSION:=0.12.07
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://codeload.github.com/ColinIanKing/stress-ng/tar.gz/refs/tags/V0.12.07
PKG_MD5SUM:=603dfce327acb220851d32f176af5b90

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=COPYING

#PKG_INSTALL:=1

include ${INCLUDE_DIR}/package.mk

TARGET_CFLAGS += -pthread
TARGET_LDFLAGS += -lrt -ldl -lz -lcrypt

define Package/stress-ng
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+zlib
  TITLE:=stress-ng is a more powerful stress utility
  URL:=http://kernel.ubuntu.com/~cking/stress-ng/
endef

define Package/stress-ng/description
stress-ng is a powerful tool that imposes certain types of compute \ stress on
UNIX-like operating systems.
endef

define Package/$(PKG_NAME)/config
	if PACKAGE_$(PKG_NAME)
	menu "stress-ng build config"
		config BUILD_STRESS_NG_STATIC
			bool
			prompt "build by static"
			default n

		config BUILD_STRESS_NG_VERBOSE
			bool
			prompt "verbose when building"
			default n
	endmenu
	endif
endef


ifeq ($(CONFIG_BUILD_STRESS_NG_STATIC), y)
TARGET_BUILD_STRESS_NG_STATIC  := 1
TARGET_LDFLAGS += $(if $(CONFIG_USE_GLIBC),-lc -lgcc -lgcc_eh)
endif
ifeq ($(CONFIG_BUILD_STRESS_NG_STATIC), y)
TARGET_BUILD_STRESS_NG_VERBOSE := y
endif


define Build/Compile
	#stress-ng
	$(MAKE) -C $(PKG_BUILD_DIR)/ \
		ARCH="$(TARGET_ARCH)" \
		AR="$(TARGET_AR)" \
		CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" \
		TARGET_CFLAGS="$(TARGET_CFLAGS)" \
		TARGET_LDFLAGS="$(TARGET_LDFLAGS)" \
		STATIC="$(TARGET_BUILD_STRESS_NG_STATIC)" \
		VERBOSE="$(TARGET_BUILD_STRESS_NG_VERBOSE)" \
		stress-ng
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/stress-ng  $(1)/usr/bin
endef

$(eval $(call BuildPackage,stress-ng))
