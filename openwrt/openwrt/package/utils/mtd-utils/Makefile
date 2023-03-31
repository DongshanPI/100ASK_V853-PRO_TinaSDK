#
# Copyright (C) 2009-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mtd-utils
PKG_VERSION:=2.1.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=ftp://ftp.infradead.org/pub/mtd-utils/
PKG_HASH:=8ad4c5f34716d40646aa28724a2f5616d325a6f119254f914e26976f1f76e9d6

PKG_INSTALL:=1
PKG_FIXUP:=autoreconf

PKG_FLAGS:=nonshared

PKG_BUILD_DEPENDS:=util-linux

PKG_LICENSE:=GPLv2
PKG_LICENSE_FILES:=

PKG_MAINTAINER:=John Crispin <john@phrozen.org>

include $(INCLUDE_DIR)/package.mk

FILES:= \
	docfdisk \
	doc_loadbios \
	flashcp \
	flash_erase \
	flash_eraseall:+mtd-utils-flash-erase \
	flash_lock \
	flash_otp_dump \
	flash_otp_info \
	flash_otp_lock \
	flash_otp_write \
	flash_unlock \
	ftl_check \
	ftl_format \
	jffs2dump:+zlib:+liblzo \
	jffs2reader:+zlib:+liblzo \
	lsmtd \
	mkfs.jffs2:+zlib:+liblzo \
	mkfs.ubifs:+libuuid:+liblzo:+zlib \
	mtd_debug \
	mtdinfo \
	mtdpart \
	nanddump \
	nandtest \
	nandwrite \
	nftldump \
	nftl_format \
	recv_image \
	rfddump \
	rfdformat \
	serve_image \
	sumtool \
	ubiattach \
	ubiblock \
	ubicrc32 \
	ubidetach \
	ubiformat \
	ubihealthd \
	ubimkvol \
	ubinfo \
	ubinize \
	ubirename \
	ubirmvol \
	ubirsvol \
	ubiupdatevol

define Package/mtd-utils/Default
  SECTION:=utils
  CATEGORY:=Utilities
  URL:=http://www.linux-mtd.infradead.org/
endef

define Package/ubi-utils
 $(call Package/mtd-utils/Default)
  DEPENDS+=@!PACKAGE_mtd-utils
  TITLE:=Utilities for ubi info/debug
endef

define Package/ubi-utils/description
  Utilities for manipulating memory technology devices.
endef

define Package/nand-utils
 $(call Package/mtd-utils/Default)
  DEPENDS+=@!PACKAGE_mtd-utils
  TITLE:=Utilities for nand flash erase/read/write/test
endef

define Package/nand-utils/description
  Utilities for NAND devices.
endef

MAKE_FLAGS += LDLIBS+="$(LIBGCC_S)"

CONFIGURE_ARGS += \
	--disable-tests \
	--without-crypto \
	--without-xattr \
	--without-zstd

TARGET_CFLAGS += -ffunction-sections -fdata-sections
TARGET_LDFLAGS += -Wl,--gc-sections

define Package/ubi-utils/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/sbin/{ubiattach,ubicrc32,ubiblock,ubidetach,ubiformat,ubimkvol} $(1)/usr/sbin/
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/sbin/{ubinfo,ubinize,ubirename,ubirmvol,ubirsvol,ubiupdatevol} $(1)/usr/sbin/
endef

define Package/nand-utils/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) \
	$(PKG_INSTALL_DIR)/usr/sbin/{flash_erase,nanddump,nandwrite,nandtest,mtdinfo} $(1)/usr/sbin/
endef

#$(eval $(call BuildPackage,ubi-utils))
#$(eval $(call BuildPackage,nand-utils))

define Package/mtd-utils
 $(call Package/mtd-utils/Default)
  TITLE:=Utilities for flash info/debug
  MENU:=1
endef

define Package/mtd-utils/description
  Utilities for manipulating memory technology devices.
endef

define Package/mtd-utils/install
	true
endef

define Build/InstallDev
	$(INSTALL_DIR)  $(STAGING_DIR)/lib
	$(INSTALL_DIR)  $(STAGING_DIR)/usr/include/mtd
	$(INSTALL_BIN)  $(PKG_BUILD_DIR)/libmtd.a $(STAGING_DIR)/lib
	$(INSTALL_BIN)  $(PKG_BUILD_DIR)/libubi.a $(STAGING_DIR)/lib
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/include/libubi.h $(STAGING_DIR)/usr/include/mtd
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/include/mtd/*.h $(STAGING_DIR)/usr/include/mtd
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/include/libmtd.h $(STAGING_DIR)/usr/include/mtd
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/include/mtd_swab.h $(STAGING_DIR)/usr/include/mtd
endef

define PartGen
define Package/mtd-utils-$(subst _,-,$(firstword $(subst :, ,$(1))))
 $(call Package/mtd-utils/Default)
  TITLE:=$(firstword $(subst :, ,$(1))) package from mtd-utils
  DEPENDS:=mtd-utils $(wordlist 2,$(words $(subst :, ,$(1))),$(subst :, ,$(1)))
endef
endef

$(foreach file,$(FILES),$(eval $(call PartGen,$(file))))

define PartInstall
define Package/mtd-utils-$(subst _,-,$(firstword $(subst :, ,$(1))))/install
	$(INSTALL_DIR) $$(1)/usr/sbin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/sbin/$(firstword $(subst :, ,$(1))) \
		$$(1)/usr/sbin/
endef
endef

$(foreach file,$(FILES),$(eval $(call PartInstall,$(file))))

$(eval $(call BuildPackage,mtd-utils))
$(foreach file,$(FILES),$(eval $(call BuildPackage,mtd-utils-$(subst _,-,$(firstword $(subst :, ,$(file)))))))

