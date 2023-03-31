Package/xr819s-firmware = $(call Package/firmware-default,Xradio xr819s firmware)

PKG_CONFIG_DEPENDS +=CONFIG_XR819S_USE_40M_SDD

define Package/xr819s-firmware/install
	$(INSTALL_DIR) $(1)/$(FIRMWARE_PATH)
	$(INSTALL_DATA) $(TOPDIR)/package/firmware/linux-firmware/xr819s/boot_xr819s.bin $(1)/$(FIRMWARE_PATH)/boot_xr819s.bin
	$(INSTALL_DATA) $(TOPDIR)/package/firmware/linux-firmware/xr819s/etf_xr819s.bin $(1)/$(FIRMWARE_PATH)/etf_xr819s.bin
	$(INSTALL_DATA) $(TOPDIR)/package/firmware/linux-firmware/xr819s/fw_xr819s.bin $(1)/$(FIRMWARE_PATH)/fw_xr819s.bin
ifeq ($(CONFIG_XR819S_USE_40M_SDD), y)
	$(INSTALL_DATA) $(TOPDIR)/package/firmware/linux-firmware/xr819s/sdd_xr819s-40M.bin $(1)/$(FIRMWARE_PATH)/sdd_xr819s.bin
	$(INSTALL_DATA) $(TOPDIR)/package/firmware/linux-firmware/xr819s/fw_xr819s_bt_40M.bin $(1)/$(FIRMWARE_PATH)/fw_xr819s_bt.bin
else
	$(INSTALL_DATA) $(TOPDIR)/package/firmware/linux-firmware/xr819s/sdd_xr819s-24M.bin $(1)/$(FIRMWARE_PATH)/sdd_xr819s.bin
	$(INSTALL_DATA) $(TOPDIR)/package/firmware/linux-firmware/xr819s/fw_xr819s_bt.bin $(1)/$(FIRMWARE_PATH)/fw_xr819s_bt.bin
endif
endef
$(eval $(call BuildPackage,xr819s-firmware))
