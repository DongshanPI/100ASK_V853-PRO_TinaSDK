#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=linux-firmware
PKG_RELEASE:=1

PKG_MAINTAINER:=Allwinner
PKG_CONFIG_DEPENDS := CONFIG_FIRMWARE_PATH CONFIG_XR829_USE_40M_SDD CONFIG_XR819S_USE_40M_SDD

SCAN_DEPS = *.mk

ifeq ($(CONFIG_FIRMWARE_PATH),)
FIRMWARE_PATH:=/lib/firmware/
else
FIRMWARE_PATH:=$(CONFIG_FIRMWARE_PATH)
endif

include $(INCLUDE_DIR)/package.mk

define firmware/config
$(info Config:
  config FIRMWARE_PATH
  string
  prompt "Firmware's directory"
  default "/lib/firmware/"
  help
     Configure the location of firmware for system.

  config XR829_USE_40M_SDD
  bool
  prompt "xr829 with 40M sdd"
  default n
  help
     Configure to Y to choose 40M firmware for XR829

  config XR819S_USE_40M_SDD
  bool
  prompt "xr819s with 40M sdd"
  depends on PACKAGE_xr819s-firmware
  default n
  help
     Configure to Y to choose 40M firmware for XR819S

  config USES_BCMDHD
	bool

  config USES_XRADIO
	bool

  config USES_XR829
	bool

  config USES_XR819S
	bool

  config USES_REALTEK
	bool

  config USES_ESP8089
	bool
@@
)
endef

define Package/firmware-default
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=$(1)
endef


define Build/Compile

endef


include $(wildcard */*.mk)
include $(wildcard ./*.mk)

ifneq ($(DUMP),)
$(call firmware/config,)
endif

#$(eval $(call BuildPackage,linux-firmware))
