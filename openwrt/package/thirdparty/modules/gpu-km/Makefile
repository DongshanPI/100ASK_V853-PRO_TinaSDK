#
# Copyright (C) 2018 Allwinner Technology Limited. All rights reserved
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# Author: Albert Yu <yuxyun@allwinnertech.com>
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
-include $(TINA_BUILD_TOP)/.config
-include machinfo/$(LICHEE_IC)/common/build.mk

TINA_TARGET_KERNEL_VERSION:=$(subst linux-,,$(LICHEE_KERN_VER))
TINA_TARGET_ARCH:=$(ARCH)

ifeq ($(CONFIG_COMPLILE_KERNEL64_USER32),y)
LINUX_KARCH=arm64
TARGET_CROSS=aarch64-openwrt-linux-
endif

MAKE_OPTS:=\
	CROSS_COMPILE=$(TARGET_CROSS) \
	KDIR=$(LINUX_DIR) \
	KERNELDIR=$(LINUX_DIR) \
	LICHEE_PLATFORM=linux \
	LICHEE_KDIR=$(LINUX_DIR)

ifeq ($(TINA_TARGET_ARCH), arm)
MAKE_OPTS+=\
	ARCH=arm
else ifeq ($(TINA_TARGET_ARCH), aarch64)
MAKE_OPTS+=\
	ARCH=arm64
else ifeq ($(TINA_TARGET_ARCH), riscv)
$(error Invalid TINA_TARGET_ARCH)
else
$(warning Invalid TINA_TARGET_ARCH)
endif

ifeq ($(PKG_NAME), mali-utgard-km)
ifeq ($(TINA_TARGET_KERNEL_VERSION), $(filter $(TINA_TARGET_KERNEL_VERSION),3.4))
	MAKE_OPTS+=USING_DT=0
else
	MAKE_OPTS+=USING_DT=1
endif
endif

PKG_RELEASE:=2

include $(INCLUDE_DIR)/package.mk

define KernelPackage/mali-utgard-km
	SUBMENU:=Video Support
	TITLE:=Mali Utgard Device Driver
#	DEPENDS:=+WESTON_DRM:kmod-sunxi-drm +!WESTON_DRM:kmod-sunxi-disp
	FILES:=$(GPU_KO_DIR)
	AUTOLOAD:=$(call AutoLoad,50,mali)
	KCONFIG:=
endef

define KernelPackage/mali-midgard-km
	SUBMENU:=Video Support
	TITLE:=Mali Midgard device driver
#	DEPENDS:=+WESTON_DRM:kmod-sunxi-drm +!WESTON_DRM:kmod-sunxi-disp
	FILES:=$(GPU_KO_DIR)
	AUTOLOAD:=$(call AutoLoad,50,mali_kbase)
	KCONFIG:=
endef

define KernelPackage/sgx544-km
	SUBMENU:=Video Support
	TITLE:=SGX-544 Device Driver
#	DEPENDS:= +kmod-sunxi-disp
	FILES:=$(GPU_KO_DIR)
	AUTOLOAD:=$(call AutoLoad,20,pvrsrvkm,1)
	AUTOLOAD+=$(call AutoLoad,25,dc_sunxi,1)
	KCONFIG:=
endef

define KernelPackage/ge8300-km
	SUBMENU:=Video Support
	TITLE:=GE8300 Device Driver
#	DEPENDS:= +kmod-sunxi-disp
	FILES:=$(GPU_KO_DIR)
	AUTOLOAD:=$(call AutoLoad,20,pvrsrvkm,1)
	AUTOLOAD+=$(call AutoLoad,25,dc_sunxi,1)
	KCONFIG:=
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -j -C $(GPU_BUILD_DIR) $(MAKE_OPTS)
endef

$(eval $(if $(PKG_NAME),$(call KernelPackage,$(PKG_NAME),,)))
