
include $(LICHEE_TOP_DIR)/openwrt/build/tinahooks_variable.mk
include $(LICHEE_TOP_DIR)/openwrt/build/tinahooks_commonfunc.mk

define TinaHooks/CopyFromDefconfig
	echo copy from $(TINA_TARGET_DEFCONFIG)
	[ -f "$(TINA_TARGET_DEFCONFIG)" ] && \
		cp $(TINA_TARGET_DEFCONFIG) .config || \
		echo "Can't find defconfig"
endef

define TinaHooks/CopyToDefconfig
	echo copy to $(TINA_TARGET_DEFCONFIG)
	[ -f "$(TINA_TARGET_DEFCONFIG)" ] && \
		cp .config $(TINA_TARGET_DEFCONFIG) || \
		echo "Can't find defconfig"
endef

define TinaHooks/PrepareTmpdir
	mkdir -p $(TARGET_OUT_DIR)/tmp ;\
	$(call TinaHooks/ForceCreateSoftLink,\
		$(TARGET_OUT_DIR)/tmp, $(TOPDIR)/tmp)
endef


define TinaHooks/PrepareStagingdir
	mkdir -p $(TARGET_OUT_DIR)/staging_dir ;\
	$(call TinaHooks/ForceCreateSoftLink,\
		$(TARGET_OUT_DIR)/staging_dir, $(TOPDIR)/staging_dir)
endef

define TinaHooks/PrepareMkbootimg
	[ ! -e $(TARGET_OUT_DIR)/staging_dir/host/bin/ ] && mkdir -p $(TARGET_OUT_DIR)/staging_dir/host/bin/ ; \
	[ -e "$(LICHEE_TOP_DIR)/tools/pack/pctools/linux/android/mkbootimg" ] && \
		cp $(LICHEE_TOP_DIR)/tools/pack/pctools/linux/android/mkbootimg $(TARGET_OUT_DIR)/staging_dir/host/bin/
endef


.PHONY: tinahooks/createtargetoutdir tinahooks/target/linux

tinahooks/createtargetoutdir: FORCE
	[ -n  "$(TARGET_OUT_DIR)" -a ! -e "$(TARGET_OUT_DIR)" ] && mkdir -p $(TARGET_OUT_DIR) ;\
		exit 0
	$(call TinaHooks/PrepareMkbootimg)


tinahooks/target/linux: FORCE
	if [ -e "$(TINA_TARGET_LINUX_DIR)" ]; then \
		rm -rf  $(TOPDIR)/target/linux ; \
		ln -s $(TINA_TARGET_LINUX_DIR) $(TOPDIR)/target/linux ; \
	fi



