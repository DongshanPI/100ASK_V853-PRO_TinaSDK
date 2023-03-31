#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tslib
PKG_VERSION:=1.22
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_MD5SUM:=aa0f409d6f4a7ccdbeee9d869558d6ee
PKG_BUILD_DIR:=$(BUILD_DIR)/tslib-$(PKG_VERSION)
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

PKG_CONFIG_DEPENDS := \
	CONFIG_TSLIB_ENABLE_pthres \
	CONFIG_TSLIB_ENABLE_variance \
	CONFIG_TSLIB_ENABLE_dejitter \
	CONFIG_TSLIB_ENABLE_linear

define Package/tslib
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=tslib is an abstraction layer for touchscreen panel events
  URL:=https://github.com/kergoth/tslib
  MENU:=1
endef

define Package/tslib/description
  tslib is an abstraction layer for touchscreen panel events,
  as well as a filter stack for the manipulation of those events.
  Examples of implemented filters include jitter smoothing and the calibration transform.
endef

define Package/tslib/config
	source "$(SOURCE)/Config.in"
endef

define Build/Prepare
	if [ -d "patches" ]; then \
		cp patches/ patches_tmp -rfd; \
	fi

	if [ -d "${TARGET_PLATFORM}-patches" ]; then \
		mkdir -p patches; \
		cp ${TARGET_PLATFORM}-patches/* patches/; \
	fi
	$(Build/Prepare/Default)

	rm patches/ -rf
	if [ -d "patches_tmp" ]; then \
		mv patches_tmp patches; \
	fi
endef

define Package/tslib/conffiles
	/etc/tslib-env.sh
	/etc/ts.conf
	/etc/pointercal
endef

EXTRA_CFLAGS+= -D__linux__

CONFIGURE_ARGS += \
        $(if $(CONFIG_TSLIB_ENABLE_galax), --enable-galax, --disable-galax) \
        $(if $(CONFIG_TSLIB_ENABLE_cy8mrln-palmpre), --enable-cy8mrln-palmpre, --disable-cy8mrln-palmpre) \
        $(if $(CONFIG_TSLIB_ENABLE_dmc_dus3000), --enable-dmc_dus3000, --disable-dmc_dus3000) \
        $(if $(CONFIG_TSLIB_ENABLE_arctic2), --enable-arctic2, --disable-arctic2) \
        $(if $(CONFIG_TSLIB_ENABLE_collie), --enable-collie, --disable-collie) \
        $(if $(CONFIG_TSLIB_ENABLE_corgi), --enable-corgi, --disable-corgi) \
        $(if $(CONFIG_TSLIB_ENABLE_linear-h2200), --enable-linear-h2200, --disable-linear-h2200) \
        $(if $(CONFIG_TSLIB_ENABLE_dmc), --enable-dmc, --disable-dmc) \
        $(if $(CONFIG_TSLIB_ENABLE_h3600), --enable-h3600, --disable-h3600) \
        $(if $(CONFIG_TSLIB_ENABLE_mk712), --enable-mk712, --disable-mk712) \
        $(if $(CONFIG_TSLIB_ENABLE_ucb1x00), --enable-ucb1x00, --disable-ucb1x00) \
        $(if $(CONFIG_TSLIB_ENABLE_waveshare), --enable-waveshare, --disable-waveshare) \
        $(if $(CONFIG_TSLIB_ENABLE_touchkit), --enable-touchkit, --disable-touchkit) \
        $(if $(CONFIG_TSLIB_ENABLE_tatung), --enable-tatung, --disable-tatung) \
        $(if $(CONFIG_TSLIB_ENABLE_variance), --enable-variance, --disable-variance) \
        $(if $(CONFIG_TSLIB_ENABLE_invert), --enable-invert, --disable-invert) \
        $(if $(CONFIG_TSLIB_ENABLE_lowpass), --enable-lowpass, --disable-lowpass) \
        $(if $(CONFIG_TSLIB_ENABLE_skip), --enable-skip, --disable-skip) \
        $(if $(CONFIG_TSLIB_ENABLE_pthres), --enable-pthres, --disable-pthres) \
        $(if $(CONFIG_TSLIB_ENABLE_median), --enable-median, --disable-median) \
        $(if $(CONFIG_TSLIB_ENABLE_linear), --enable-linear, --disable-linear) \
        $(if $(CONFIG_TSLIB_ENABLE_iir), --enable-iir, --disable-iir) \
        $(if $(CONFIG_TSLIB_ENABLE_dejitter), --enable-dejitter, --disable-dejitter) \
        $(if $(CONFIG_TSLIB_ENABLE_debounce), --enable-debounce, --disable-debounce) \
        $(if $(CONFIG_TSLIB_ENABLE_input), --enable-input, --disable-input)  \
	$(if $(CONFIG_TSLIB_ENABLE_tools), --enable-tools, --disable-tools) \
	$(if $(CONFIG_TSLIB_ENABLE_tests), --enable-tests, --disable-tests) \
	--enable-shared

TARGET_LDFLAGS+= $(if $(CONFIG_USE_GLIBC),-ldl)

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include \
		$(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/** $(1)/usr/lib/pkgconfig
endef

define Package/tslib/install
	$(INSTALL_DIR) $(1)/usr/lib/ts \
		$(1)/usr/bin \
		$(1)/etc
	{  \
	    set +e; \
	    $(CP) $(PKG_INSTALL_DIR)/usr/bin/ts_* $(1)/usr/bin/;  \
	    $(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/;  \
	    $(CP) $(PKG_INSTALL_DIR)/usr/lib/ts/*.so* $(1)/usr/lib/ts/; \
	    $(CP) ./files/ts.conf $(1)/etc/ts.conf; \
	    $(CP) ./files/tslib-env.sh $(1)/etc/; \
	}
endef

$(eval $(call BuildPackage,tslib))
