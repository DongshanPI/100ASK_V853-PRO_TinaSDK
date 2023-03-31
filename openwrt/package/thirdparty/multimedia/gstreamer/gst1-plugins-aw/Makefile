#
# Copyright (C) 2017-2020 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gst1-plugins-aw
PKG_VERSION:=1.8.2
PKG_RELEASE:=1

PKG_MAINTAINER:=Harm Hanemaaijer<fenfb@yahoo.com>
PKG_LICENSE:=LGPLv2
PKG_LICENSE_FILES:=COPYING

PKG_BUILD_DIR:=$(BUILD_DIR)/gst-plugins-aw-$(PKG_VERSION)
PKG_SOURCE:=gst-plugins-aw-$(PKG_VERSION).tar.xz
PKG_MD5SUM:=

PKG_BUILD_DEPENDS:= libgstreamer1 gstreamer1-plugins-base gstreamer1-plugins-good linux
PKG_CONFIG_DEPENDS:=

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

KERNEL_VERSION = CONF_KERNEL_VERSION_3_10

#config the kernel version,which depends on $(KERNEL_PATCHVER)
ifeq ($(KERNEL_PATCHVER),3.4)
    KERNEL_VERSION = CONF_KERNEL_VERSION_3_4
endif

ifeq ($(KERNEL_PATCHVER),3.10)
    KERNEL_VERSION = CONF_KERNEL_VERSION_3_10
endif

ifeq ($(KERNEL_PATCHVER),4.4)
    KERNEL_VERSION = CONF_KERNEL_VERSION_4_4
endif

ifeq ($(KERNEL_PATCHVER),4.9)
    KERNEL_VERSION = CONF_KERNEL_VERSION_4_9
endif

ifeq ($(KERNEL_PATCHVER),5.4)
    KERNEL_VERSION = CONF_KERNEL_VERSION_5_4
endif
#end config the kernel version

GST_AW_LIBS:=
GST_AW_MODULES:=

define Package/gstreamer1-aw/Default
  CATEGORY:=Multimedia
  SECTION:=multimedia
  SUBMENU:=GStreamer
  TITLE:=GStreamer
  URL:=http://gstreamer.freedesktop.org/
  DEPENDS:= +libgstreamer1 $(ICONV_DEPENDS) +libuapi
endef

define Package/gstreamer1-aw/description/Default
  GStreamer open source multimedia framework
endef


define Package/gst1-plugins-aw
$(call Package/gstreamer1-aw/Default)
  TITLE+= plugins collection (aw)
  DEPENDS+= $(GST_DEPENDS) +libuapi
  HIDDEN:=1
endef

define Package/gts-plugins-aw/description
$(call Package/gstreamer1-aw/description/Default)
 .
 This meta package contains only dependencies to the other plugins from
 the aw plugins collection.
endef

define Package/gstreamer1-plugins-aw
  $(call Package/gstreamer1-aw/Default)
  TITLE+= plugins collection (aw)
  DEPENDS:=+ALL:gst1-plugins-aw +gstreamer1-plugins-base +libuapi
endef

define Package/gstreamer1-plugins-aw/config
    menu "Select GStreamer aw modules"
      depends on PACKAGE_gstreamer1-plugins-aw


    config gst1-plugins-aw-all
      bool "Include all GStreamer aw plugins"
      select PACKAGE_gst1-plugins-aw

    comment "Modules"

   $(foreach mod,$(GST_AW_MODULES), \
    config PACKAGE_gst1-mod-$(mod)
      prompt "GStreamer $(mod) module"

   )

   config G2D_ROTATE
   bool "sunxi g2d rotation support"
   depends on PACKAGE_gst1-mod-sunxifbsink
   default n
   help
     Choose Y to use rotate module of g2d.

    comment "Libraries"

   $(foreach lib,$(GST_AW_LIBS), \
    config PACKAGE_libgst1$(lib)
      prompt "GStreamer $(lib) library"

   )

  endmenu

endef

GST_COND_SELECT = $(patsubst %,$(if $(CONFIG_PACKAGE_gst1-mod-$(1)),--enable,--disable)-%,$(1))

GST_VERSION:=1.0

CONFIGURE_ARGS += \
	--disable-debug \
	--disable-examples \
	\
	\
	--without-libiconv-prefix \
	--without-libintl-prefix \
	--without-x \

ifeq ($(CONFIG_SUNXI_DISPLAY_TWO),y)
TARGET_CFLAGS+=-D__SUNXI_DISPLAY2__
TARGET_CFLAGS+=-D$(KERNEL_VERSION)
endif
ifeq ($(CONFIG_SUNXI_DISPLAY_ONE),y)
TARGET_CFLAGS+=-D__SUNXI_DISPLAY__
TARGET_CFLAGS+=-D$(KERNEL_VERSION)
endif
ifeq ($(CONFIG_G2D_ROTATE),y)
TARGET_CFLAGS+=-D__SUNXI_G2D_ROTATE__
endif

TARGET_CFLAGS+= -std=gnu99 -Wno-error=format-security
EXTRA_LDFLAGS+= \
	-Wl,-rpath-link=$(STAGING_DIR)/usr/lib \
	$(if $(ICONV_FULL),-liconv) \


define Package/gst1-plugins-aw/install
	/bin/true
endef

define Package/gstreamer1-plugins-aw/install
	/bin/true
endef

# 1: short name
# 2: description
# 3: dependencies on other gstreamer libraries (short name)
# 4: dependencies on other packages
define GstBuildLibrary

  GST_DEPENDS += +libgst1$(1)

  GST_AW_LIBS+= $(1)
  define Package/libgst1$(1)
    $(call Package/gstreamer1-aw/Default)
    TITLE+= $(2) library (base)
    DEPENDS+= $$(foreach p,$(3),+libgst1$$(p)) $(4)
    HIDDEN:=1
  endef

  define Package/libgst1$(1)/description
   $(call Package/gstreamer1-aw/description/Default)
   .
   This package contains the GStreamer $(2) library.
  endef

  define Package/libgst1$(1)/install
	$(INSTALL_DIR) $$(1)/usr/lib
	( cd $(PKG_INSTALL_DIR); $(CP) \
		./usr/lib/libgst$(1).so \
		$$(1)/usr/lib/ \
	)
  endef

  $$(eval $$(call BuildPackage,libgst1$(1)))
endef

#$(eval $(call GstBuildLibrary,framebuffersink,fbdev sink,controller video libgstreamer1,,))
$(eval $(call GstBuildLibrary,framebuffersink,sunxi fbdev support,controller video,,))

# 1: short name
# 2: description
# 3: dependencies on other gstreamer libraries (short name)
# 4: dependencies on other gstreamer plugins (short name)
# 5: dependencies on other packages
define GstBuildPlugin

  GST_DEPENDS += +gst1-mod-$(1)

  GST_AW_MODULES+= $(1)

  define Package/gst1-mod-$(1)
    $(call Package/gstreamer1-aw/Default)
    TITLE+= $(2) plugin (aw)
    DEPENDS+= +libuapi +libgstreamer1 $$(foreach p,$(3),+libgst1$$(p)) $$(foreach p,$(4),+gst1-mod-$$(p)) $(5)
    HIDDEN:=1
  endef

  define Package/gst1-mod-$(1)/description
   $(call Package/gstreamer1-aw/description/Default)
   .
   This package contains the GStreamer $(2) plugin.
  endef

  define Package/gst1-mod-$(1)/install
	$(INSTALL_DIR) $$(1)/usr/lib/gstreamer-$(GST_VERSION)
	( cd $(PKG_INSTALL_DIR); $(CP) \
		./usr/lib/gstreamer-$(GST_VERSION)/libgst$(1).so \
		$$(1)/usr/lib/gstreamer-$(GST_VERSION)/ \
	)
  endef

  $$(eval $$(call BuildPackage,gst1-mod-$(1)))
endef

$(eval $(call GstBuildPlugin,fbdev2sink,sunxi fbdev support,controller framebuffersink video,,))
#$(eval $(call GstBuildPlugin,framebuffersink,sunxi fbdev support,video,,))
$(eval $(call GstBuildPlugin,sunxifbsink,sunxi fbdev support,controller framebuffersink video,,))

$(eval $(call BuildPackage,gstreamer1-plugins-aw))
$(eval $(call BuildPackage,gst1-plugins-aw))
